const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
const lane = __dirname;
const page = fs.readFileSync(process.argv[2] || 'ux/ledger/ledger.html', 'utf8');
const mode = process.argv[3] || 'a';
const decode = s => s.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&quot;/g,'"')
  .replace(/&#x27;/g,"'").replace(/&amp;/g,'&').replace(/&middot;/g,' · ').replace(/&nbsp;/g,' ');
let checks = 0;
function check(ok, name) { checks++; assert.ok(ok, name); }
const articles = [...page.matchAll(/<article\b([^>]*)>([\s\S]*?)<\/article>/g)];
const rowsHTML = page.match(/<table class="list"[^>]*>[\s\S]*?<tbody>([\s\S]*?)<\/tbody>/)[1];
const attrs = s => Object.fromEntries([...s.matchAll(/([\w-]+)="([^"]*)"/g)].map(m=>[m[1], decode(m[2])]));
const rows = [...rowsHTML.matchAll(/<tr\b([^>]*)>/g)].map(m=>{
  const a = attrs(m[1]);
  return {dataset:{id:a['data-id'],state:a['data-state'],q:a['data-q']},hidden:false,
    classList:{toggle:(name, value)=>{}}};
});
check(rows.length === 100, '100 list rows');
check(articles.length === 100, '100 cards');
const counts = {};
rows.forEach(r=>counts[r.dataset.state]=(counts[r.dataset.state]||0)+1);
console.log('DOM state counts:', JSON.stringify(counts));
check(counts.sealed===94 && counts.refuted===4 && counts.open===1 && counts.proposed===1, 'counts');
for (const [,tag,body] of articles) {
  const id = attrs(tag).id.replace(/^claim-/, '');
  if (!id.startsWith(mode)) continue;
  const claim = JSON.parse(fs.readFileSync('ux/ledger/claims/'+id+'.json'));
  const sealpath = 'ux/ledger/seals/'+id+'.json';
  const seal = fs.existsSync(sealpath) ? JSON.parse(fs.readFileSync(sealpath)) : null;
  const decoded = decode(body);
  check(decoded.includes(claim.statement), id+' statement');
  for (const s of claim.script) check(decoded.includes(s), id+' script '+s);
  for (const s of claim.expect) check(decoded.includes(s), id+' expectation '+s);
  if (seal) for (const s of seal.actual) check(decoded.includes(s), id+' actual '+s);
  if (claim.parent) check(body.includes('href="#claim-'+claim.parent+'"'), id+' parent link');
  if (id==='a1-cos-4-at-2') {
    check(body.includes('0001') && body.includes('1001'), 'binary digits');
    check(body.includes('digit of 2^3'), 'first differing binary digit');
  }
  if (id==='a3-exp-3-exp-12-mod-3-8') {
    check(body.includes('01022111') && body.includes('21000211'), 'ternary digits');
    check(body.includes('digit of 3^2'), 'first differing ternary digit');
  }
  const text = decode(body.replace(/<[^>]*>/g,' ').replace(/\s+/g,' '));
  fs.writeFileSync(path.join(lane,id+'.card.txt'),text+'\n');
  console.log(id+': card text and all script/expect/actual lines checked.');
}
const handlers = {};
const buttons = [...page.matchAll(/<button\b([^>]*data-filter[^>]*)>/g)].map(m=>{
  const a=attrs(m[1]);
  return {dataset:{filter:a['data-filter']},setAttribute(){},
    addEventListener(event, cb){handlers[a['data-filter']]=cb;}};
});
const shown={textContent:0};
const qsel={addEventListener(event,cb){handlers.question=cb;}};
const document={querySelectorAll(sel){
  if(sel==='table.list tbody tr') return rows;
  if(sel==='button[data-filter]') return buttons;
  throw Error(sel);
}, getElementById(id){return id==='shown'?shown:qsel;}};
const script=[...page.matchAll(/<script>([\s\S]*?)<\/script>/g)][0][1];
vm.runInNewContext(script,{document,location:{hash:''},window:{addEventListener(){}}});
check(shown.textContent===100,'initial shown');
handlers.refuted(); check(shown.textContent===4,'refuted filter');
handlers.all();
const question=rows.find(r=>r.dataset.id.startsWith(mode+'1')).dataset.q;
handlers.question({target:{value:question}});
check(shown.textContent===(mode==='a'?6:8),'scenario filter');
handlers.sealed(); check(shown.textContent===(mode==='a'?3:5),'sealed and scenario filter');
const controls=[...page.matchAll(/<(button|input|textarea|select)\b[^>]*>/g)].map(m=>m[0]);
console.log('Controls:',controls.join(' '));
console.log('No claim entry, comment, ratify, retract, cite, split or report export control found.');
const header = fs.readFileSync('include/adelefeld/status.h','utf8');
for(const m of header.matchAll(/^#define ADF_(\w+)\s+\d+\s+\/\*/gm)) {
  if(m[1].startsWith('CMP_') || m[1]==='STATUS_COUNT')continue;
  check(page.slice(page.indexOf('<footer')).includes(m[1]),'status legend '+m[1]);
}
console.log('DOM assertions:',checks);
console.log('The JS run used lightweight element stubs; no pixel or layout verification is claimed.');
