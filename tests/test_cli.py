"""Keep CLI documentation/version metadata aligned; optionally check a Windows build."""

import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "pd" / "pd.cpp").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
VERSION_HEADER = (ROOT / "pd" / "version.h").read_text(encoding="utf-8")
VERSION = re.search(r'#define PD_VERSION_STRING "([^"]+)"', VERSION_HEADER)[1]
HELP = re.search(r'R"help\((.*?)\)help"', SOURCE, re.S)[1]
FLAGS = set(re.findall(r'lstrcmp\(\s*argv\[i\]\s*,\s*L"([^"]+)"\)', SOURCE))
DB_COMMANDS = set(re.findall(r'lstrcmp\(\s*argv\[i\s*\+\s*1\]\s*,\s*L"([^"]+)"\)', SOURCE))


class CliDocumentationTests(unittest.TestCase):
    def test_version_is_shared_with_resources_and_readme(self):
        numeric = re.search(r"#define PD_VERSION_RESOURCE ([\d,]+)", VERSION_HEADER)[1]
        self.assertEqual(numeric, VERSION.replace(".", ",") + ",0")
        resource = (ROOT / "pd" / "pd.rc").read_text(encoding="utf-8")
        for directive in ("FILEVERSION", "PRODUCTVERSION"):
            self.assertRegex(resource, rf"{directive}\s+PD_VERSION_RESOURCE")
        for field in ("FileVersion", "ProductVersion"):
            self.assertIn(f'VALUE "{field}", PD_VERSION_STRING ".0"', resource)
        self.assertIn('printf("Process Dump v%s\\n", PD_VERSION_STRING)', SOURCE)
        self.assertIn(f"# Process Dump {VERSION}\n", README)
        self.assertIn(f"## Version {VERSION} (unreleased)", README)

    def test_every_parser_flag_and_database_command_is_documented(self):
        self.assertGreater(len(FLAGS), 20)
        self.assertEqual(DB_COMMANDS, {"gen", "genquick", "add", "remove", "rem", "clean", "ignore"})
        for document in (HELP, README):
            for flag in FLAGS:
                with self.subTest(flag=flag, document="help" if document == HELP else "README"):
                    self.assertRegex(document, rf"(?<![\w-]){re.escape(flag)}(?![\w-])")
            for command in DB_COMMANDS:
                self.assertIn(f"-db {command}", document)

    def test_help_describes_defaults_and_important_scope(self):
        options = (ROOT / "pd" / "simple.h").read_text(encoding="utf-8")
        threads = re.search(r"NumberOfThreads\((\d+)\)", options)[1]
        self.assertIn(f"default: {threads}", HELP)
        self.assertIn(f"default worker count is {threads}", README)
        for text in ("%USERPROFILE%", "requires -pid", "ALL THREE", "not -db gen",
                     "Last -t/-nt wins", "-nep takes precedence", "default: off",
                     "256 MiB", "never overwritten", "Does not run the dump"):
            self.assertIn(text, HELP)
        self.assertNotIn("%HOMEPATH%", HELP)
        self.assertNotIn("%HOMEPATH%", README)
        self.assertNotIn("(?i)", HELP)


@unittest.skipUnless(os.name == "nt" and os.environ.get("PD_TEST_EXE"),
                     "set PD_TEST_EXE to a built Windows pd.exe for CLI integration checks")
class CliIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.executable = Path(os.environ["PD_TEST_EXE"]).resolve(strict=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="pd-cli-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.copy = self.root / "pd.exe"
        shutil.copyfile(self.executable, self.copy)

    def run_help(self, *arguments):
        result = subprocess.run([str(self.copy), *arguments], cwd=self.root, capture_output=True,
                                text=True, errors="replace", timeout=15, check=True)
        self.assertEqual(result.stderr, "")
        self.assertNotIn("Finished running.", result.stdout)
        self.assertNotIn("WARNING:", result.stdout)
        return result.stdout

    def test_help_aliases_banner_and_suppression(self):
        for arguments in ((), ("--help",), ("-help",), ("-h",), ("--h",)):
            with self.subTest(arguments=arguments):
                output = self.run_help(*arguments)
                self.assertTrue(output.startswith(f"Process Dump v{VERSION}\n"))
                self.assertTrue(output.endswith(HELP))
        self.assertEqual(self.run_help("--help", "-nh"), HELP)
        self.assertEqual({p.name for p in self.root.iterdir()}, {"pd.exe"})

    def test_help_does_not_execute_database_maintenance(self):
        originals = {}
        for name in ("clean.hashes", "entrypoints.hashes", "shortentrypoints.hashes"):
            originals[name] = struct.pack("<Q", 123)
            (self.root / name).write_bytes(originals[name])
        for arguments in (("--help", "-db", "clean"), ("-db", "clean", "-h")):
            self.run_help(*arguments)
            self.assertEqual({name: (self.root / name).read_bytes() for name in originals}, originals)
        self.assertEqual({p.name for p in self.root.iterdir()}, {"pd.exe", *originals})

    def test_windows_file_and_product_versions(self):
        env = dict(os.environ, PD_TEST_EXE=str(self.executable))
        command = ("(Get-Item -LiteralPath $env:PD_TEST_EXE).VersionInfo | "
                   "Select-Object FileVersion,ProductVersion,FileMajorPart,FileMinorPart,"
                   "FileBuildPart,FilePrivatePart,ProductMajorPart,ProductMinorPart,"
                   "ProductBuildPart,ProductPrivatePart | ConvertTo-Json -Compress")
        result = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
                                env=env, capture_output=True, text=True, check=True, timeout=30)
        info = json.loads(result.stdout)
        for kind in ("File", "Product"):
            self.assertEqual(info[f"{kind}Version"], VERSION + ".0")
            self.assertEqual([info[f"{kind}{part}Part"] for part in ("Major", "Minor", "Build", "Private")],
                             [*map(int, VERSION.split(".")), 0])


if __name__ == "__main__":
    unittest.main()
