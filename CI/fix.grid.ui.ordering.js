const fs = require("fs");
const sax = require("sax");

const GRID_LAYOUT_CLASS = "QGridLayout";

const uiDir = `${__dirname}/../src/ui`;

class Layout {
    items = []

    constructor(type) {
        this.type = type
    }

    addItem(item) {
        this.items.push(item)
    }
}

class Item {
    row
    column
    startOffset
    endOffset
    content

    constructor(startOffset, row, column) {
        this.startOffset = startOffset
        this.row = parseInt(row);
        this.column = parseInt(column);
    }

    getCellOrder() {
        return this.row * 1000 + this.column;
    }

    isValid() {
        return this.row !== undefined && this.column !== undefined
    }

}

function visitUiFile(file) {
    const filePath = `${uiDir}/${file}`
    const stack = []
    const processed = []

    let uiFileContents = fs.readFileSync(filePath, 'utf-8');

    const saxStream = sax.createStream(false, {})
    let lastTagClosed = 0
    saxStream.onopentag = (node) => {
        if (node.name === 'LAYOUT') {
            stack.push(new Layout(node.attributes.CLASS))
        }
        if (node.name === 'ITEM') {
            stack.push(new Item(lastTagClosed, node.attributes.ROW, node.attributes.COLUMN))
        }

    }
    saxStream.onclosetag = (tag) => {
        if (tag === 'ITEM') {
            const item = stack.pop()
            if (!item.isValid() && stack.at(-1) instanceof Layout) {
                return
            }
            item.endOffset = saxStream._parser.position
            item.content = uiFileContents.substring(item.startOffset, item.endOffset)
            if (stack.at(-1) instanceof Layout) {
                stack.at(-1).addItem(item)
            }
        }
        if (tag === 'LAYOUT') {
            processed.push(stack.pop())
        }
        lastTagClosed = saxStream._parser.position
    }

    saxStream.end = () => {

        processed.filter(layout => layout.type === GRID_LAYOUT_CLASS).forEach((layout) => {
            let indexOffset = 0
            let endPosition = 0;
            layout.items.forEach(item => {
                uiFileContents = uiFileContents.substring(0, item.startOffset + indexOffset) + uiFileContents.substring(item.endOffset + indexOffset)
                indexOffset -= (item.endOffset - item.startOffset)
                endPosition = Math.max(indexOffset, item.endOffset)
            })

            uiFileContents = uiFileContents.substring(0, endPosition + indexOffset) + layout.items.sort((a, b) => a.getCellOrder() - b.getCellOrder()).map(item => item.content).join("") + uiFileContents.substring(endPosition + indexOffset)
        })
        fs.writeFileSync(filePath, uiFileContents)

    }
    fs.createReadStream(filePath).pipe(saxStream)

}

fs.readdirSync( uiDir ).forEach(visitUiFile)
