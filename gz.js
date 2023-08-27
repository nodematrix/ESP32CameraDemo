// ESP32CameraDemo - https://nodematrix.cc
// Copyright (c) 2023 Node Matrix
// MIT License

/**
 * Convert html file to gzip compressed c header file.
 * Usage: node gz
 */
const fs = require('fs');
const zlib = require('zlib');

let path = './';
let list = ['index']; // list of files to be converted

for (let n = 0; n < list.length; n++) {
    let name = list[n];
    console.log(`convert ${name}.html to gzip compressed ${name}.h`);

    let src = fs.readFileSync(`${path}${name}.html`, { encoding: 'utf8', flag: 'r' });
    console.log(`html length: ${src.length}`);

    // trim
    src = src.replace(/^ +/gm, '').replace(/(\n\r*\n)/g, '\n');

    // gzip
    var gz = zlib.gzipSync(src);
    let len = gz.length;
    console.log(`gzip length: ${len}`);

    // write .h
    let h = `#define ${name}_gz_len ${len}\n\n`;
    h += `const unsigned char ${name}_gz[] = {`;
    for (let i = 0; i < len; i++) {
        if (i % 16 == 0) {
            h += '\n    ';
        }
        h += ('0x' + gz.toString('hex', i, i + 1));
        if (i < (len - 1)) {
            h += ',';
            if (i % 16 != 15) h += ' ';
        }
    }
    h += '};';

    fs.writeFileSync(`${path}${name}.h`, h, { encoding: 'utf8' });
    console.log('done\n');
}