const assert = require('assert');
const path = require('path');
const fs = require('fs-promise');

const rreaddir = async (dir, result = []) => {
    const files = (await fs.readdir(dir)).map(f => path.join(dir, f));
    result.push(...files);
    await Promise.all(files.map(async f => (
      (await fs.stat(f)).isDirectory() && rreaddir(f, result)
    )));
    return result;
};

const create_tabs = (indent, multiline) => {
    if (!multiline) {
        return "";
    }

    return " ".repeat(indent*2);
};

const guard = (f, x, indent, multiline) => {
    if (!multiline) {
        return f(x, indent, false);
    }

    const result = f(x, indent, false);
    if (result.length < 80) {
        return result;
    }

    return f(x, indent, true)
};

const render_value = (value, indent, multiline) => {
    if (Array.isArray(value)) {
        return guard(render_tuple, value, indent, multiline);
    }

    if (value === null) {
        return 'null';
    }

    switch (typeof value) {
        case 'object':
            return guard(render_variant, value, indent, multiline);
        case 'string':
            return `"${value}"`;
        case 'number':
            return value;
        default:
            return '?';
    }
};

const render_tuple = (array, indent, multiline) => {
    assert(Array.isArray(array));
    const tabs = create_tabs(indent, multiline);
    const newline = multiline ? "\n" : " ";

    let result = '';

    result += `(${newline}`;
    result += array.map(value => create_tabs(indent+1, multiline) + guard(render_value, value, indent+1, multiline)).join(`,${newline}`)+newline;
    result += `${tabs})`;

    return result;
};

const render_variant = (variant, indent = 0, multiline = true) => {
    assert.equal(typeof variant, 'object');
    assert(!Array.isArray(variant));
    assert.equal(Object.entries(variant).length, 1);

    const [key, values] = Object.entries(variant)[0];
    const tabs = create_tabs(indent, multiline);
    const newline = multiline ? "\n" : "";

    let result = '';

    result += `${key}(${newline}`;
    result += values.map(value => create_tabs(indent+1, multiline) + guard(render_value, value, indent+1, multiline)).join(`,${newline}`)+newline;
    result += `${tabs})`;

    return result;
};

const main = async () => {
    const paths = (await rreaddir("./repo")).filter(
        path => path.endsWith(".json")
    );

    for (const path of paths) {
        const type = JSON.parse(await (fs.readFile(path)));
        const str = render_variant(type);

        await fs.writeFile(path.replace('.json', '.txt'), str);
    }
};

main();