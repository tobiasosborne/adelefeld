
(function () {
  var state = 'all', q = 'all';
  var rows = Array.prototype.slice.call(document.querySelectorAll('table.list tbody tr'));
  function apply() {
    var shown = 0;
    rows.forEach(function (r) {
      var ok = (state === 'all' || r.dataset.state === state) && (q === 'all' || r.dataset.q === q);
      r.hidden = !ok;
      if (ok) shown++;
    });
    document.getElementById('shown').textContent = shown;
  }
  document.querySelectorAll('button[data-filter]').forEach(function (b) {
    b.addEventListener('click', function () {
      state = b.dataset.filter;
      document.querySelectorAll('button[data-filter]').forEach(function (x) {
        x.setAttribute('aria-pressed', x === b ? 'true' : 'false');
      });
      apply();
    });
  });
  document.getElementById('qsel').addEventListener('change', function (e) { q = e.target.value; apply(); });
  function mark() {
    var id = decodeURIComponent(location.hash.replace(/^#claim-/, ''));
    rows.forEach(function (r) { r.classList.toggle('sel', r.dataset.id === id); });
  }
  window.addEventListener('hashchange', mark);
  mark();
  apply();
})();
