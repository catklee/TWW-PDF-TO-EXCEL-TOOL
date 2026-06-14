# TWW-PDF-TO-EXCEL-TOOL

**Author:** Catherine Kim Lee ([catklee@umich.edu](mailto:catklee@umich.edu))  
**Built with:** C++ and native SpreadsheetML (`.xls`)

## Why?

In case you didn't know, I'm actually a small business owner — check out [TheWelcomeWeek.com](https://thewelcomeweek.com).

The vendor for my sock business has a pretty unfriendly workflow for global buyers. There is a pseudo-checkout for international buyers that doesn't actually work. After a few back-and-forth emails, they asked me to hand-type every product name, option, quantity, and price into an Excel sheet and email it back—all while their site actively blocks copying and pasting from the cart. 🤯💀

Looking long-term, I decided to tackle this bottleneck over the summer and built this tool.

The workflow is simple: print your cart page to a PDF, then run this tool on that file. It reads the layout-preserved text from the PDF (via `pdftotext`) and automatically generates an Excel spreadsheet you can send over in a matter of seconds.

## Prerequisites

- **OS:** macOS (Terminal)
- **Dependency:** `pdftotext` (via the Poppler library)

To install the dependency using Homebrew, run:

```bash
brew install poppler
```

## Build

```bash
git clone https://github.com/catklee/TWW-PDF-TO-EXCEL-TOOL.git
cd TWW-PDF-TO-EXCEL-TOOL
make
```

## Use

1. Shop and add items to your cart as you normally would.
2. Print the page (`Cmd+P`) and choose **Save as PDF**.
3. Run:

```bash
./cart_pdf_to_excel ~/Downloads/yourfilename.pdf
```

4. This writes `yourfilename_cart.xls` to your **Desktop**. Excel opens it directly.

Custom output path:

```bash
./cart_pdf_to_excel ~/Downloads/yourfilename.pdf ~/Desktop/cart.xls
```

## Notes

- The tool reads the printable PDF layout, not the live website.
- Product name and option are exported in separate columns.
- Handles cart items that span a page break in the PDF.
- If `pdftotext` is installed somewhere else, set `PDFTOTEXT=/path/to/pdftotext`.
