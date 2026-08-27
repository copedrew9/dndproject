#!/usr/bin/env python3
"""Extract the text layer of the sourcebook PDFs into TextFiles/.

TextFiles/ used to hold OCR of scanned pages, which was lossy in ways that
cost real work: it confused C with G and I with L, lost spaces inside words,
read two-column stat blocks across the gutter, and dropped whole tables --
the Player's Handbook equipment table did not survive it at all, names
included.

The PDFs carry a text layer, and it is clean. This reads that instead, so
TextFiles/ says what the books say. `make audit` then checks every name in
data/ against it.

This is run once, against PDFs that are not in the repository: they are 255MB
and they are not ours to distribute. The extracted text is what is checked in.
The script is kept so that anyone holding the books can reproduce it.

    pip install pymupdf
    python3 tools/pdf_text.py path/to/pdfs

Reading order, not layout, is what comes out: a table's cells arrive one per
line, column by column within a row, which is enough both to search for a
name and to read a row back off. Where a value is missing from the text layer
-- and a few are -- rendering the cell to an image is the only recourse, and
that is noted where it was needed.
"""

import os
import sys

try:
    import fitz                      # PyMuPDF
except ImportError:
    sys.exit("build: pip install pymupdf")

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

# The PDF each dump comes from. Mordenkainen Presents: Monsters of the
# Multiverse is not among them: TextFiles/MORDENKAIDENtext.txt is still the
# older OCR, and is left alone.
BOOKS = [
    ("Player's Handbook.pdf",                "PHBtext.txt"),
    ("Xanathar's Guide to Everything.pdf",   "XANATHARtext.txt"),
    ("Tasha's Cauldron of Everything.pdf",   "TASHAtext.txt"),
    ("Dungeon Master's Guide.pdf",           "DMGtext.txt"),
    ("Monster Manual.pdf",                   "MMtext.txt"),
]


def extract(pdf_path, out_path):
    doc = fitz.open(pdf_path)
    parts = []
    for n, page in enumerate(doc):
        parts.append("\n=== page %d ===\n" % (n + 1))
        parts.append(page.get_text())
    doc.close()
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("".join(parts))
    return len("".join(parts))


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Books")
    if not os.path.isdir(src):
        sys.exit("no such directory: %s\n"
                 "Pass the directory holding the PDFs as an argument." % src)

    for pdf, out in BOOKS:
        path = os.path.join(src, pdf)
        if not os.path.exists(path):
            print("skipping %s (not found)" % pdf)
            continue
        n = extract(path, os.path.join(ROOT, "TextFiles", out))
        print("%-44s -> TextFiles/%-22s %8d characters" % (pdf, out, n))


if __name__ == "__main__":
    main()
