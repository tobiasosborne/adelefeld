// Read the actual HTML and execute its inline script against a minimal DOM adapter.
// This checks structure and event behavior, not browser layout or CSS appearance.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const decode = s => s.replace(/&quot;/g, '"').replace(/&#x27;/g, "'")
  .replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&amp;/g, '&');
for (const file of process.argv.slice(2)) {
  const html = fs.readFileSync(file, 'utf8');
  const recordPath = file.replace(/\.html$/, '.records.jsonl');
  const records = fs.readFileSync(recordPath, 'utf8').trim().split('\n').map(JSON.parse);
  const wells = records.filter(r => r.kind === 'well' && r.output != null);
  const spans = [...html.matchAll(/<span class="w ([^"]+)" id="([^"]+)" title="([^"]*)">([^<]*)<\/span>/g)];
  const byId = new Map(spans.map(m => [m[2], {classes:m[1], title:decode(m[3]), text:decode(m[4])}]));
  let checked = 0;
  for (const r of wells) {
    const w = byId.get(`w-${r.row}-${r.column}`);
    assert.ok(w, `missing ${r.row}, ${r.column}`);
    assert.ok(w.title.includes(`line: ${r.line}`));
    assert.ok(w.title.includes(`prec ${r.prec}`));
    assert.ok(w.title.includes(`output: ${r.output}`));
    assert.ok(w.title.includes(`status: ${r.status}`));
    assert.ok(w.classes.split(' ').includes(({open:'nd',limit:'lim'})[r.class] || r.class));
    if (r.count !== null) assert.equal(w.text, String(r.count));
    checked++;
  }
  function elem(attrs = {}) {
    return {attrs, children:[], events:{}, className:'', textContent:'',
      classList:{lit:false, toggle(k,v) {this[k]=v;}},
      getAttribute(k) { return this.attrs[k]; },
      addEventListener(k,f) {this.events[k]=f;},
      appendChild(e) {this.children=this.children.filter(x=>x!==e);this.children.push(e);e.parent=this;},
      remove() {this.parent.children=this.parent.children.filter(x=>x!==this);},
      querySelectorAll(s) {return this.children.filter(e=>s==='tr.grp' ? e.className==='grp' : 'data-p' in e.attrs);}
    };
  }
  const tbody = elem();
  const rows = [...html.matchAll(/<tr data-p="(\d+)"/g)].map(m=>elem({'data-p':m[1]}));
  rows.forEach(r=>tbody.appendChild(r));
  const options = [...html.matchAll(/<option value="(\d+)"/g)].map(m=>m[1]);
  const sel = elem(); sel.value='0';
  sel.querySelector=s=>options.includes(/value="(\d+)"/.exec(s)[1]) ? {} : null;
  const finds = [...html.matchAll(/<div class="find"[^>]*data-cells="([^"]*)"/g)]
    .map(m=>elem({'data-cells':m[1]}));
  const cells = new Map([...byId.keys()].map(id=>[id,elem()]));
  for (const f of finds) for (const id of f.attrs['data-cells'].split(' ').filter(Boolean)) {
    assert.ok(cells.has(id), `finding references missing well ${id}`);
  }
  const context={location:{hash:options.includes('8') ? '#mod8' : ''},document:{
    getElementById:id=>id==='wall'?tbody:id==='order'?sel:cells.get(id),
    querySelectorAll:()=>finds, createElement:()=>elem()}};
  const scripts=[...html.matchAll(/<script>([\s\S]*?)<\/script>/g)];
  assert.equal(scripts.length,1);
  vm.runInNewContext(scripts[0][1], context, {timeout:1000});
  let regroupChecks=0;
  for (const m of options.map(Number).filter(Boolean)) {
    sel.value=String(m);sel.events.change();
    const actual=tbody.children.filter(e=>'data-p' in e.attrs).map(e=>Number(e.attrs['data-p']));
    const key=p=>p===0 || m%p===0 ? -1 : p%m;
    const expected=rows.map(e=>Number(e.attrs['data-p'])).sort((a,b)=>key(a)-key(b));
    assert.deepEqual(actual,expected);
    assert.equal(tbody.children.filter(e=>e.className==='grp').length,new Set(expected.map(key)).size);
    regroupChecks++;
  }
  sel.value='0';sel.events.change();assert.deepEqual(tbody.children,rows);
  for (const f of finds) {
    const ids=f.attrs['data-cells'].split(' ').filter(Boolean);
    for (const event of ['mouseenter','focusin']) {
      f.events[event]();for (const id of ids) assert.equal(cells.get(id).classList.lit,true);
    }
    for (const event of ['mouseleave','focusout']) {
      f.events[event]();for (const id of ids) assert.equal(cells.get(id).classList.lit,false);
    }
  }
  console.log(JSON.stringify({file,rows:rows.length,wells:spans.length,records_checked:checked,
    findings:finds.length,regroup_checks:regroupChecks,stop:decode(/<div class="stop">([\s\S]*?)<\/div>/
      .exec(html)[1].replace(/<[^>]+>/g,' ')),hash_path:context.location.hash}));
}
