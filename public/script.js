const expressionEl = document.getElementById('expression');
const resultEl = document.getElementById('result');
const statusEl = document.getElementById('status');
const historyEl = document.getElementById('history');
const angleToggle = document.getElementById('angleToggle');
let expression = '';
let angle = 'DEG';
let history = [];

function render() {
  expressionEl.textContent = expression || '0';
}
function insert(value) {
  expression += value;
  render();
  statusEl.textContent = 'READY';
}
function clearAll() {
  expression = '';
  resultEl.textContent = '0';
  statusEl.textContent = 'READY';
  render();
}
function deleteLast() {
  expression = expression.slice(0, -1);
  render();
}
function showError(msg) {
  resultEl.textContent = 'Error';
  statusEl.textContent = msg.toUpperCase().slice(0, 30);
}
function renderHistory() {
  if (!history.length) {
    historyEl.innerHTML = '<div class="empty">nothing here yet</div>';
    return;
  }
  historyEl.innerHTML = '';
  history.slice().reverse().forEach(item => {
    const div = document.createElement('button');
    div.className = 'history-item';
    div.innerHTML = `<small>${esc(item.expression)}</small><strong>${esc(item.result)}</strong>`;
    div.addEventListener('click', () => {
      expression = item.result;
      resultEl.textContent = item.result;
      render();
    });
    historyEl.appendChild(div);
  });
}
function esc(s) {
  return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

// math engine
function factorial(x) {
  if (x < 0 || x > 170 || Math.floor(x) !== x) throw new Error('bad factorial input');
  let r = 1;
  for (let i = 2; i <= x; i++) r *= i;
  return r;
}

function calc(op, a, b) {
  const deg = angle === 'DEG';
  const pi = Math.PI;
  const x = deg ? a * pi / 180 : a;

  switch (op) {
    case 'add': return a + b;
    case 'subtract': return a - b;
    case 'multiply': return a * b;
    case 'divide':
      if (b === 0) throw new Error('div by zero');
      return a / b;
    case 'power': return Math.pow(a, b);
    case 'mod':
      if (b === 0) throw new Error('div by zero');
      return a % b;
    case 'sin': return Math.sin(x);
    case 'cos': return Math.cos(x);
    case 'tan':
      if (Math.abs(Math.cos(x)) < 1e-12) throw new Error('undefined');
      return Math.tan(x);
    case 'asin':
      if (a < -1 || a > 1) throw new Error('domain error');
      { const r = Math.asin(a); return deg ? r * 180 / pi : r; }
    case 'acos':
      if (a < -1 || a > 1) throw new Error('domain error');
      { const r = Math.acos(a); return deg ? r * 180 / pi : r; }
    case 'atan':
      { const r = Math.atan(a); return deg ? r * 180 / pi : r; }
    case 'sinh': return Math.sinh(a);
    case 'cosh': return Math.cosh(a);
    case 'tanh': return Math.tanh(a);
    case 'sqrt':
      if (a < 0) throw new Error('negative sqrt');
      return Math.sqrt(a);
    case 'cbrt': return Math.cbrt(a);
    case 'square': return a * a;
    case 'cube': return a * a * a;
    case 'log':
      if (a <= 0) throw new Error('log domain');
      return Math.log10(a);
    case 'ln':
      if (a <= 0) throw new Error('ln domain');
      return Math.log(a);
    case 'exp': return Math.exp(a);
    case 'factorial': return factorial(a);
    case 'abs': return Math.abs(a);
    case 'reciprocal':
      if (a === 0) throw new Error('div by zero');
      return 1 / a;
    case 'percent': return a / 100;
    case 'pi': return pi;
    case 'e': return Math.E;
    default: throw new Error('unknown op');
  }
}

function api(op, a, b) {
  b = b || 0;
  const r = calc(op, a, b);
  if (!Number.isFinite(r)) throw new Error('math error');
  return r;
}

// tokenizer
function tokenize(input) {
  const s = input.replace(/π/g, 'pi').replace(/\be\b/g, 'e').replace(/×/g, '*').replace(/÷/g, '/');
  const tokens = [];
  let i = 0;
  while (i < s.length) {
    if (/\s/.test(s[i])) { i++; continue; }
    if (/[0-9.]/.test(s[i])) {
      let j = i;
      while (j < s.length && /[0-9.]/.test(s[j])) j++;
      const n = Number(s.slice(i, j));
      if (!Number.isFinite(n)) throw new Error('bad number');
      tokens.push({type:'number', value:n}); i=j; continue;
    }
    if (/[a-z]/i.test(s[i])) {
      let j=i; while(j<s.length && /[a-z]/i.test(s[j])) j++;
      const w=s.slice(i,j).toLowerCase();
      if (['pi','e'].includes(w)) tokens.push({type:'constant', value:w});
      else throw new Error('use the buttons for functions');
      i=j; continue;
    }
    if ('+-*/^()%'.includes(s[i])) { tokens.push({type:'operator', value:s[i]}); i++; continue; }
    throw new Error('unexpected character');
  }
  return tokens;
}

function toRpn(tokens) {
  const out=[], stack=[];
  const prec={'u-':4,'^':3,'*':2,'/':2,'%':2,'+':1,'-':1};
  let prev='start';
  for (const t of tokens) {
    if (t.type==='number'||t.type==='constant') { out.push(t); prev='value'; }
    else if (t.value==='(') { stack.push(t); prev='left'; }
    else if (t.value===')') {
      while(stack.length&&stack[stack.length-1].value!=='(') out.push(stack.pop());
      if(!stack.length) throw new Error('mismatched parens');
      stack.pop(); prev='value';
    } else {
      let op=t.value;
      if(op==='-'&&(prev==='start'||prev==='operator'||prev==='left')) op='u-';
      while(stack.length&&stack[stack.length-1].value!=='('&&
        ((op!=='^'&&prec[stack[stack.length-1].value]>=prec[op])||
         (op==='^'&&prec[stack[stack.length-1].value]>prec[op]))) out.push(stack.pop());
      stack.push({type:'operator',value:op}); prev='operator';
    }
  }
  while(stack.length) {
    const t=stack.pop(); if(t.value==='(') throw new Error('mismatched parens'); out.push(t);
  }
  return out;
}

function evalExpr(input) {
  const rpn=toRpn(tokenize(input)), stk=[];
  for(const t of rpn) {
    if(t.type==='number') stk.push(t.value);
    else if(t.type==='constant') stk.push(api(t.value,0));
    else {
      const op=t.value;
      if(op==='u-') { if(!stk.length) throw new Error('bad expression'); stk.push(-stk.pop()); continue; }
      if(stk.length<2) throw new Error('bad expression');
      const b=stk.pop(), a=stk.pop();
      const map={'+':'add','-':'subtract','*':'multiply','/':'divide','^':'power','%':'mod'};
      stk.push(api(map[op],a,b));
    }
  }
  if(stk.length!==1) throw new Error('bad expression');
  return stk[0];
}

function doUnary(op) {
  if (!expression) throw new Error('type a number first');
  const val = evalExpr(expression);
  const result = api(op, val);
  const old = expression;
  const fmt = format(result);
  expression = fmt;
  resultEl.textContent = fmt;
  statusEl.textContent = 'OK';
  history.push({expression: old + ' → ' + op, result: fmt});
  renderHistory(); render();
}

function format(n) {
  if (!Number.isFinite(n)) return 'Error';
  return Number(n.toPrecision(12)).toString();
}

function equals() {
  if (!expression) return;
  const old = expression;
  try {
    const result = evalExpr(old);
    const fmt = format(result);
    resultEl.textContent = fmt;
    statusEl.textContent = 'OK';
    history.push({expression: old, result: fmt});
    if (history.length > 20) history.shift();
    renderHistory();
    expression = fmt;
    render();
  } catch (err) { showError(err.message); }
}

// wire up buttons
document.querySelectorAll('[data-insert]').forEach(btn =>
  btn.addEventListener('click', () => insert(btn.dataset.insert)));

document.querySelectorAll('[data-action]').forEach(btn =>
  btn.addEventListener('click', () => {
    try { doUnary(btn.dataset.action); } catch(e) { showError(e.message); }
  }));

document.querySelectorAll('[data-command]').forEach(btn =>
  btn.addEventListener('click', () => {
    if (btn.dataset.command === 'clear') clearAll();
    if (btn.dataset.command === 'delete') deleteLast();
    if (btn.dataset.command === 'equals') equals();
  }));

angleToggle.addEventListener('click', () => {
  angle = angle === 'DEG' ? 'RAD' : 'DEG';
  angleToggle.textContent = angle;
  document.getElementById('modeLabel').textContent = angle;
});

document.getElementById('clearHistory').addEventListener('click', () => {
  history = [];
  renderHistory();
});

document.addEventListener('keydown', e => {
  if (/^[0-9.+\-*/^()%]$/.test(e.key)) insert(e.key);
  else if (e.key === 'Enter' || e.key === '=') { e.preventDefault(); equals(); }
  else if (e.key === 'Backspace') deleteLast();
  else if (e.key === 'Escape') clearAll();
});

render();
