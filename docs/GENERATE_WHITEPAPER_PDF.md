# Whitepaper PDF Generation Guide

## Overview

The whitepaper PDF was removed because it contained outdated information (700 Trillion DNT supply and pricing). The markdown source has been updated with the correct information (10 Billion DNT, no pricing).

## How to Regenerate the PDF

The markdown source file is located at:
```
docs/DINARI_BLOCKCHAIN_WHITEPAPER.md
```

### Method 1: Using Pandoc (Recommended)

```bash
# Install pandoc if not already installed
# Ubuntu/Debian:
sudo apt-get install pandoc texlive-latex-base texlive-fonts-recommended texlive-latex-extra

# macOS:
brew install pandoc basictex

# Generate PDF
cd docs
pandoc DINARI_BLOCKCHAIN_WHITEPAPER.md \
  -o DINARI_BLOCKCHAIN_WHITEPAPER.pdf \
  --pdf-engine=pdflatex \
  --variable geometry:margin=1in \
  --variable fontsize=11pt \
  --toc \
  --highlight-style=tango
```

### Method 2: Using Markdown to PDF (Node.js)

```bash
# Install markdown-pdf
npm install -g markdown-pdf

# Generate PDF
cd docs
markdown-pdf DINARI_BLOCKCHAIN_WHITEPAPER.md
```

### Method 3: Using Online Converters

1. Go to https://www.markdowntopdf.com/ or similar
2. Upload `docs/DINARI_BLOCKCHAIN_WHITEPAPER.md`
3. Download the generated PDF
4. Save as `docs/DINARI_BLOCKCHAIN_WHITEPAPER.pdf`

### Method 4: Using VS Code Extension

1. Install "Markdown PDF" extension in VS Code
2. Open `DINARI_BLOCKCHAIN_WHITEPAPER.md`
3. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
4. Type "Markdown PDF: Export (pdf)"
5. PDF will be generated in the same directory

## Verification

After generating the PDF, verify it contains:
- ✅ **10 Billion DNT** as initial supply (NOT 700 Trillion)
- ✅ **NO pricing information** (no price tables, investment tiers, etc.)
- ✅ All other information from the markdown file

## Current Status

- ✅ Markdown file updated with correct information
- ❌ PDF removed (needs regeneration)
- 📝 Use any method above to regenerate the PDF

---

**Last Updated**: 2025-10-31
**Reason for Removal**: Outdated supply (700T) and pricing information
**Source**: `docs/DINARI_BLOCKCHAIN_WHITEPAPER.md`
