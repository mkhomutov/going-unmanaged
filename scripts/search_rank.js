// Ask the site's own search worker the questions in scripts/search_queries.tsv.
//
// scripts/check_search.sh drives this; SITE-PLAN.md is why the site exists.
//
// Not a reimplementation: this loads the very worker the published site loads
// (build/site/assets/javascripts/workers/search.*.min.js) and hands it the
// very index the browser fetches, then talks its protocol - `{type: 0}` to
// set up, `{type: 2}` to query, `{type: 3}` back with the results. So the
// ranking checked here is the ranking a reader gets, including every part of
// it nobody should have to know about: the wildcard Material appends to each
// term, the post-query boost that rewards a section matching all of them, and
// the grouping that makes the results a list of pages rather than sections.
//
// The first draft of this file did reimplement it, on top of lunr, and got
// three things wrong that changed the answer - length normalisation, the
// wildcard (every term, not just the last) and the boost. Rewriting it this
// way removed the whole class of mistake, and the npm dependency with it. A
// Material upgrade needs nothing here; if one ever changes the worker's
// globals, the shim below fails loudly rather than scoring differently.
//
// Usage: node search_rank.js <site-dir> <queries.tsv> [top-n]
//        node search_rank.js <site-dir> --query "some words" [n]
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const [, , siteDir, second, third] = process.argv;

const workerDir = path.join(siteDir, "assets", "javascripts", "workers");
const workerFile = fs.readdirSync(workerDir).find((name) => /^search\..*\.min\.js$/.test(name));
if (!workerFile) {
    console.error(`search_rank.js: no search worker under ${workerDir} - did the site build?`);
    process.exit(1);
}
const index = JSON.parse(
    fs.readFileSync(path.join(siteDir, "search", "search_index.json"), "utf8"));

// The worker expects a worker global scope. Everything it actually touches is
// here; `importScripts` stays a stub because it is only reached for a language
// other than English, which this site does not configure.
let onMessage = null;
const outbox = [];
const scope = {
    console,
    location: { href: `http://localhost/assets/javascripts/workers/${workerFile}` },
    addEventListener: (type, fn) => { if (type === "message") onMessage = fn; },
    postMessage: (message) => outbox.push(message),
    importScripts: () => {
        throw new Error("the search worker asked to import a language script; "
            + "this site is configured for English only");
    },
    setTimeout, clearTimeout, Promise, TextDecoder, TextEncoder, URL,
};
scope.self = scope;
scope.globalThis = scope;
vm.createContext(scope);
vm.runInContext(fs.readFileSync(path.join(workerDir, workerFile), "utf8"), scope);
if (!onMessage) {
    console.error("search_rank.js: the search worker registered no message handler - "
        + "Material's worker protocol has changed, and this script needs rereading");
    process.exit(1);
}

async function post(message) {
    outbox.length = 0;
    await onMessage({ data: message });
    return outbox[outbox.length - 1];
}

// `suggest` only decides whether the worker also returns completions for the
// search box; it does not touch the ranking.
async function setup() {
    const reply = await post({
        type: 0,
        data: { config: index.config, docs: index.docs, options: { suggest: false } },
    });
    if (!reply || reply.type !== 1) {
        throw new Error("the search worker did not acknowledge its setup message");
    }
}

// Results arrive grouped by page, best group first, each group's first item
// being the page or its best-scoring section - what the reader sees.
async function search(query) {
    const reply = await post({ type: 2, data: query });
    if (!reply || reply.type !== 3) {
        throw new Error(`the search worker answered a query with ${JSON.stringify(reply)}`);
    }
    return (reply.data.items || []).map((group) => group.map((item) => item.location));
}

(async () => {
    await setup();

    if (second === "--query") {
        const limit = Number(third) || 10;
        const groups = await search(process.argv[5] || process.argv[4] || "");
        for (const [i, locations] of groups.slice(0, limit).entries()) {
            console.log(`${String(i + 1).padStart(2)}. ${locations[0]}`);
        }
        return;
    }

    // Fixture mode. Each line is `query <TAB> expected-location-prefix` and,
    // optionally, `<TAB> n` to widen the tolerance for a query the corpus
    // cannot rank higher - which records the limit rather than hiding it.
    // Blank lines and `#` comments are skipped.
    const top = Number(third) || 3;
    let failures = 0;
    let checked = 0;
    for (const line of fs.readFileSync(second, "utf8").split("\n")) {
        if (!line.trim() || line.startsWith("#")) continue;
        const [query, expected, tolerance] = line.split("\t").map((s) => (s || "").trim());
        const limit = Number(tolerance) || top;
        if (!query || !expected) {
            console.log(`  BAD LINE  ${line}`);
            failures++;
            continue;
        }
        checked++;
        const groups = await search(query);
        const at = groups.findIndex((locations) =>
            locations.some((location) => location.startsWith(expected)));
        if (at >= 0 && at < limit) continue;
        failures++;
        console.log(`  FAIL  "${query}"`);
        console.log(`        wanted ${expected} in the top ${limit}, ` + (at >= 0
            ? `it ranked ${at + 1} of ${groups.length}`
            : `it is not in the ${groups.length} results at all`));
        for (const [i, locations] of groups.slice(0, limit).entries()) {
            console.log(`        ${i + 1}. ${locations[0]}`);
        }
    }
    console.log(`search fixture: ${checked - failures}/${checked} queries land where they should`);
    process.exit(failures ? 1 : 0);
})().catch((error) => {
    console.error(`search_rank.js: ${error.message}`);
    process.exit(1);
});
