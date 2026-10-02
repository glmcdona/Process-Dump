"""Copy only the expected binary bytes from this run's untrusted artifacts."""

import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
from urllib.error import HTTPError
from urllib.parse import urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener
import zipfile


MAX_BYTES = 64 * 1024 * 1024
ARTIFACTS = {"Win32 Executable": "pd32.exe", "x64 Executable": "pd64.exe"}


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, new_url):
        return None


def bounded_read(response, limit):
    data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError("Response exceeds the release size limit")
    return data


class GitHub:
    def __init__(self):
        self.base = os.environ["GITHUB_API_URL"].rstrip("/")
        self.token = os.environ["GH_TOKEN"]
        self.opener = build_opener(NoRedirect())
        if urlsplit(self.base).scheme != "https":
            raise ValueError("The GitHub API must use HTTPS")

    def request(self, endpoint):
        return Request(self.base + endpoint, headers={
            "Authorization": "Bearer " + self.token,
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
        })

    def json(self, endpoint, allow_missing=False):
        try:
            with self.opener.open(self.request(endpoint), timeout=60) as response:
                return json.loads(bounded_read(response, 4 * 1024 * 1024))
        except HTTPError as error:
            if error.code == 404 and allow_missing:
                return None
            raise

    def archive(self, repository, artifact_id):
        endpoint = f"/repos/{repository}/actions/artifacts/{artifact_id}/zip"
        try:
            with self.opener.open(self.request(endpoint), timeout=60) as response:
                return bounded_read(response, MAX_BYTES)
        except HTTPError as error:
            if error.code != 302:
                raise
            location = error.headers["Location"]
        parsed = urlsplit(location)
        if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password:
            raise ValueError("Invalid artifact download redirect")
        # Never forward the repository token to the signed blob URL or follow another redirect.
        with self.opener.open(Request(location), timeout=60) as response:
            return bounded_read(response, MAX_BYTES)


def select_artifacts(api, repository, run_id):
    selected = {}
    for page in range(1, 101):
        listing = api.json(f"/repos/{repository}/actions/runs/{run_id}/artifacts?per_page=100&page={page}")
        entries = listing["artifacts"]
        for item in entries:
            name = item["name"]
            if name not in ARTIFACTS:
                continue
            if name in selected or item["expired"]:
                raise ValueError("Duplicate or expired release artifact")
            if type(item["id"]) is not int or item["id"] <= 0:
                raise ValueError("Invalid release artifact ID")
            if not 0 < item["size_in_bytes"] <= MAX_BYTES:
                raise ValueError("Invalid release artifact size")
            selected[name] = item
        if len(entries) < 100:
            break
    else:
        raise ValueError("Too many artifacts in the release run")
    if selected.keys() != ARTIFACTS.keys():
        raise ValueError("Both architecture artifacts are required")
    return selected


def validate_entry(entries):
    if len(entries) != 1:
        raise ValueError("A release artifact must contain exactly one entry")
    entry = entries[0]
    mode = entry.external_attr >> 16
    if (entry.filename != "pd.exe" or entry.orig_filename != "pd.exe" or entry.is_dir()
            or stat.S_IFMT(mode) not in (0, stat.S_IFREG)
            or entry.external_attr & 0x10 or entry.flag_bits & 1
            or entry.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED)
            or not 0 < entry.file_size <= MAX_BYTES
            or not 0 <= entry.compress_size <= MAX_BYTES):
        raise ValueError("Unexpected release archive entry")
    return entry


def copy_binary(archive, destination):
    if len(archive) > MAX_BYTES:
        raise ValueError("Release archive exceeds the size limit")
    with zipfile.ZipFile(io.BytesIO(archive)) as zipped:
        entry = validate_entry(zipped.infolist())
        # Validate data/CRC before writing; archive-controlled paths are never opened.
        with zipped.open(entry) as source:
            data = bounded_read(source, MAX_BYTES)
        if len(data) != entry.file_size:
            raise ValueError("Release binary length mismatch")
    with destination.open("xb") as output:
        output.write(data)
    return {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def verify_tag(api, repository, version, commit):
    ref = api.json(f"/repos/{repository}/git/ref/tags/{version}", allow_missing=True)
    if ref is None:
        return
    target = ref["object"]
    for _ in range(8):
        if target["type"] == "commit":
            if target["sha"].lower() != commit.lower():
                raise ValueError("Existing version tag does not identify the tested commit")
            return
        if target["type"] != "tag" or not re.fullmatch(r"[0-9a-fA-F]{40}", target["sha"]):
            break
        target = api.json(f"/repos/{repository}/git/tags/{target['sha']}")["object"]
    raise ValueError("Unsupported release tag target")


def main():
    repository = os.environ["GITHUB_REPOSITORY"]
    run_id = os.environ["GITHUB_RUN_ID"]
    commit = os.environ["RELEASE_COMMIT"]
    version = os.environ["RELEASE_VERSION"]
    if (not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository)
            or not re.fullmatch(r"[0-9]+", run_id)
            or not re.fullmatch(r"[0-9a-fA-F]{40}", commit)
            or not (version == "Develop" or re.fullmatch(r"v[0-9]+\.[0-9]+(\.[0-9]+)?", version))):
        raise ValueError("Invalid release identity")
    api = GitHub()
    if version != "Develop":
        verify_tag(api, repository, version, commit)
    selected = select_artifacts(api, repository, run_id)
    output = Path("release-binaries")
    output.mkdir()
    manifest = {"repository": repository, "commit": commit, "run_id": run_id, "files": {}}
    for name, filename in ARTIFACTS.items():
        artifact = selected[name]
        result = copy_binary(api.archive(repository, artifact["id"]), output / filename)
        result["artifact_id"] = artifact["id"]
        manifest["files"][filename] = result
    with (output / "provenance.json").open("x", encoding="utf-8") as destination:
        json.dump(manifest, destination, indent=2)
        destination.write("\n")


if __name__ == "__main__":
    main()
