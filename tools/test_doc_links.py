import re
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"\]\(([^)\s]+)\)")


def headings(text):
    fence = False
    for line in text.split("\n"):
        if line.lstrip().startswith("```"):
            fence = not fence
        elif not fence and re.match(r"#{1,6} ", line):
            yield line


def anchors(text):
    """Ancoras como o GitHub gera: titulo em minusculas, sem pontuacao, duplicatas com -1, -2."""
    seen, out = {}, set(re.findall(r'<a id="([^"]+)"', text))
    for line in headings(text):
        title = re.sub(r"^#{1,6} ", "", line).strip()
        title = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", title)
        title = re.sub(r"<[^>]+>", "", title).replace("`", "").replace("*", "")
        base = re.sub(r"[^\w\- ]", "", title.lower()).replace(" ", "-")
        count = seen.get(base, 0)
        out.add(base if count == 0 else f"{base}-{count}")
        seen[base] = count + 1
    return out


class DocLinkTests(unittest.TestCase):
    def tracked_docs(self):
        listed = subprocess.run(["git", "ls-files", "*.md"], cwd=ROOT, check=True, capture_output=True,
                                text=True).stdout.split()
        return [name for name in listed if (ROOT / name).exists()]

    def test_every_doc_is_english_then_portuguese(self):
        docs = self.tracked_docs()
        self.assertEqual([name for name in docs if name.endswith(".pt-BR.md")], [])
        for name in docs:
            text = (ROOT / name).read_text(encoding="utf-8")
            switch, portuguese = "English · [Português (Brasil)](#pt-br)", '<a id="pt-br"></a>'
            self.assertIn(switch, text, name)
            self.assertIn(portuguese, text, name)
            self.assertLess(text.index(switch), text.index(portuguese), name)

    def test_relative_links_and_anchors_exist(self):
        for name in self.tracked_docs():
            source = ROOT / name
            text = source.read_text(encoding="utf-8")
            for target in LINK.findall(text):
                if re.match(r"[a-z]+:", target) or target.startswith("../../"):
                    continue
                path, _, anchor = target.partition("#")
                file = (source.parent / path).resolve() if path else source
                self.assertTrue(file.exists(), f"{name}: {target}")
                if anchor and file.suffix == ".md":
                    self.assertIn(anchor, anchors(file.read_text(encoding="utf-8")), f"{name}: {target}")


if __name__ == "__main__":
    unittest.main()
