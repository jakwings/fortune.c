Installation:

    make PREFIX=/usr/local install

Uninstallation:

    make PREFIX=/usr/local uninstall

Usage:

    fortune [OPTIONS] [[N%] <FILE|FOLDER>]...

Options:

    -e       Distribute probability equally.
    -h       Show this manual.

    You can specify multiple files and folders to get cookies.
    Each file should reside with its index file <filename>.dat.
    A percentage N% changes the probability of the file/folder.
    The remaining probability will be distributed automatically.

Other:

    --index [-h] [-d <char>] [-cxs] <input> [output]
    --dump [-h] [-d <char>] [-cxs] <input> [output] [data]
    --rotate
    --version
    --help
