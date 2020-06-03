const path = require("path")

module.exports = {
    pluginPath: path
        .resolve(__dirname, "./build/Release/ngramtokenizer")
        .replace(/\\/g, '/')
}
