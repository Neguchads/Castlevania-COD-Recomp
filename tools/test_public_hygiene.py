import re
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PRIVATE_PATH = re.compile(r"(?im)\b[A-Z]:\\(?:Users\\|Recomps(?: e Mods)?\\)")
FORBIDDEN_TRACKED = (
    "assets/",
    "generated/",
    "out/",
    "config/game.toml",
    "docs/DECISIONS.md",
    "compile-local.cmd",
    "src/common/ptbr_strings.private.h",
    "tools/package_playtest.py",
    "tools/test_package_playtest.py",
)
SDK_COMMIT = "e0de3e44d10443c1c9aad512f36baf133cdd4a60"


class PublicHygieneTests(unittest.TestCase):
    def test_private_and_generated_files_are_not_tracked(self):
        result = subprocess.run(
            ["git", "ls-files", "--cached"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        tracked = result.stdout.splitlines()
        for forbidden in FORBIDDEN_TRACKED:
            self.assertFalse(
                any(path == forbidden or path.startswith(forbidden) for path in tracked),
                f"private or generated path is tracked: {forbidden}",
            )

    def test_external_sdk_is_pinned_to_the_reviewed_revision(self):
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("https://github.com/TGP482/XboxXBERecomp.git", cmake)
        self.assertIn(f"GIT_TAG {SDK_COMMIT}", cmake)

    def test_tracked_text_has_no_machine_specific_paths(self):
        result = subprocess.run(
            ["git", "ls-files", "--cached"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        for relative in result.stdout.splitlines():
            path = ROOT / relative
            try:
                content = path.read_text(encoding="utf-8")
            except (UnicodeDecodeError, OSError):
                continue
            self.assertIsNone(PRIVATE_PATH.search(content), f"machine path found in {relative}")


if __name__ == "__main__":
    unittest.main()
