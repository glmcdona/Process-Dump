import importlib.util
import io
import json
from pathlib import Path
import stat
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError
import zipfile


SPEC = importlib.util.spec_from_file_location(
    "release_artifacts", Path(__file__).resolve().parents[1] / ".github" / "scripts" / "release_artifacts.py")
release = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(release)


class FakeAPI:
    def __init__(self, replies):
        self.replies = iter(replies)
        self.calls = []

    def json(self, endpoint, **kwargs):
        self.calls.append((endpoint, kwargs))
        return next(self.replies)


class ReleaseArtifactTests(unittest.TestCase):
    def artifact(self, name, artifact_id):
        return {"name": name, "id": artifact_id, "expired": False, "size_in_bytes": 1024}

    def test_same_run_names_and_pagination(self):
        first = [self.artifact("unrelated", n + 1) for n in range(99)]
        first.append(self.artifact("Win32 Executable", 100))
        api = FakeAPI([{"artifacts": first}, {"artifacts": [self.artifact("x64 Executable", 101)]}])
        self.assertEqual(set(release.select_artifacts(api, "owner/repo", "42")), set(release.ARTIFACTS))
        self.assertEqual(api.calls[0][0], "/repos/owner/repo/actions/runs/42/artifacts?per_page=100&page=1")
        self.assertTrue(api.calls[1][0].endswith("page=2"))

    def test_missing_duplicate_expired_and_oversized_artifacts(self):
        win32 = self.artifact("Win32 Executable", 1)
        x64 = self.artifact("x64 Executable", 2)
        for entries in ([win32], [win32, x64, win32], [win32, dict(x64, expired=True)],
                        [win32, dict(x64, size_in_bytes=release.MAX_BYTES + 1)],
                        [win32, dict(x64, id=0)]):
            with self.subTest(entries=entries), self.assertRaises(ValueError):
                release.select_artifacts(FakeAPI([{"artifacts": entries}]), "owner/repo", "42")

    def test_entry_metadata_contract(self):
        entry = zipfile.ZipInfo("pd.exe")
        entry.file_size = 3
        entry.compress_size = 3
        self.assertIs(release.validate_entry([entry]), entry)
        # Metadata-only checks: no hostile archives or executable payloads are created.
        for field, value in (("filename", "unexpected.txt"), ("orig_filename", "unexpected.txt"),
                             ("external_attr", (stat.S_IFLNK | 0o777) << 16),
                             ("external_attr", (stat.S_IFDIR | 0o777) << 16),
                             ("external_attr", 0x10), ("flag_bits", 1),
                             ("compress_type", zipfile.ZIP_BZIP2),
                             ("file_size", 0), ("file_size", release.MAX_BYTES + 1)):
            original = getattr(entry, field)
            with self.subTest(field=field, value=value):
                setattr(entry, field, value)
                with self.assertRaises(ValueError):
                    release.validate_entry([entry])
                setattr(entry, field, original)
        for entries in ([], [entry, entry]):
            with self.assertRaises(ValueError):
                release.validate_entry(entries)

    def test_benign_binary_roundtrip_and_no_overwrite(self):
        data = b"benign non-executable test fixture"
        for compression in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
            archive = io.BytesIO()
            with zipfile.ZipFile(archive, "w", compression=compression) as zipped:
                zipped.writestr("pd.exe", data)
            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "fixed-name.bin"
                result = release.copy_binary(archive.getvalue(), output)
                self.assertEqual(output.read_bytes(), data)
                self.assertEqual(result, {"size": len(data), "sha256": release.hashlib.sha256(data).hexdigest()})
                with self.assertRaises(FileExistsError):
                    release.copy_binary(archive.getvalue(), output)
                self.assertEqual(output.read_bytes(), data)

    def test_invalid_archive_writes_nothing(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "fixed-name.bin"
            with self.assertRaises(zipfile.BadZipFile):
                release.copy_binary(b"not a ZIP file", output)
            self.assertFalse(output.exists())

    def test_response_size_bound(self):
        self.assertEqual(release.bounded_read(io.BytesIO(b"abc"), 3), b"abc")
        with self.assertRaises(ValueError):
            release.bounded_read(io.BytesIO(b"abcd"), 3)

    def test_tag_matches_tested_commit(self):
        commit = "a" * 40
        release.verify_tag(FakeAPI([None]), "owner/repo", "v1.2", commit)
        release.verify_tag(FakeAPI([{"object": {"type": "commit", "sha": commit}}]),
                           "owner/repo", "v1.2", commit)
        api = FakeAPI([{"object": {"type": "tag", "sha": "b" * 40}},
                       {"object": {"type": "commit", "sha": commit}}])
        release.verify_tag(api, "owner/repo", "v1.2", commit)
        self.assertEqual(api.calls[1][0], "/repos/owner/repo/git/tags/" + "b" * 40)
        with self.assertRaises(ValueError):
            release.verify_tag(FakeAPI([{"object": {"type": "commit", "sha": "b" * 40}}]),
                               "owner/repo", "v1.2", commit)

    def test_tag_api_errors_are_not_missing_tags(self):
        with patch.dict(release.os.environ, {"GITHUB_API_URL": "https://api.github.com", "GH_TOKEN": "test"}):
            api = release.GitHub()
        for status in (401, 403, 500):
            with patch.object(api.opener, "open", side_effect=HTTPError("url", status, "", {}, None)):
                with self.assertRaises(HTTPError):
                    api.json("/endpoint", allow_missing=True)
        with patch.object(api.opener, "open", side_effect=HTTPError("url", 404, "", {}, None)):
            self.assertIsNone(api.json("/endpoint", allow_missing=True))

    def test_download_redirect_drops_token(self):
        with patch.dict(release.os.environ, {"GITHUB_API_URL": "https://api.github.com", "GH_TOKEN": "test"}):
            api = release.GitHub()
        redirect = HTTPError("url", 302, "", {"Location": "https://blob.example.invalid/artifact"}, None)
        with patch.object(api.opener, "open", side_effect=[redirect, io.BytesIO(b"archive")]) as opened:
            self.assertEqual(api.archive("owner/repo", 1), b"archive")
        initial = opened.call_args_list[0].args[0]
        redirected = opened.call_args_list[1].args[0]
        self.assertEqual(initial.get_header("Authorization"), "Bearer test")
        self.assertIsNone(redirected.get_header("Authorization"))
        self.assertEqual(redirected.full_url, "https://blob.example.invalid/artifact")
        redirect.headers["Location"] = "http://blob.example.invalid/artifact"
        with patch.object(api.opener, "open", side_effect=redirect) as opened:
            with self.assertRaises(ValueError):
                api.archive("owner/repo", 1)
            self.assertEqual(opened.call_count, 1)

    def test_prepare_both_binaries_and_provenance(self):
        archive = io.BytesIO()
        with zipfile.ZipFile(archive, "w") as zipped:
            zipped.writestr("pd.exe", b"benign fixture")
        api = FakeAPI([{"artifacts": [self.artifact("Win32 Executable", 1),
                                     self.artifact("x64 Executable", 2)]}])
        api.archive = lambda repository, artifact_id: archive.getvalue()
        environment = {"GITHUB_REPOSITORY": "owner/repo", "GITHUB_RUN_ID": "42",
                       "RELEASE_COMMIT": "a" * 40, "RELEASE_VERSION": "Develop"}
        with tempfile.TemporaryDirectory() as directory, patch.dict(release.os.environ, environment):
            output = Path(directory) / "release-binaries"
            with patch.object(release, "GitHub", return_value=api), patch.object(release, "Path", return_value=output):
                release.main()
            manifest = json.loads((output / "provenance.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["commit"], "a" * 40)
            self.assertEqual(manifest["run_id"], "42")
            self.assertEqual(set(manifest["files"]), {"pd32.exe", "pd64.exe"})
            self.assertEqual(manifest["files"]["pd32.exe"]["artifact_id"], 1)
            self.assertEqual((output / "pd64.exe").read_bytes(), b"benign fixture")


if __name__ == "__main__":
    unittest.main()
