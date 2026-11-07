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
function showError(message) {
  resultEl.textContent = 'Error';
  statusEl.textContent = message.toUpperCase().slice(0, 32);
}
function renderHistory() {
  if (!history.length) {
    historyEl.innerHTML = '<div class="empty">Your calculations will appear here.</div>';
    return;
  }
  historyEl.innerHTML = '';
  history.slice().reverse().forEach(item => {
    const div = document.createElement('button');
    div.className = 'history-item';
    div.innerHTML = `<small>${escapeHtml(item.expression)}</small><strong>${escapeHtml(item.result)}</strong>`;
    div.addEventListener('click', () => {
      expression = item.result;
      resultEl.textContent = item.result;
      render();
    });
    historyEl.appendChild(div);
  });
}
function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
async function api(op, a, b = 0) {
  const response = await fetch('/api/calculate', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({op, a, b, angle})
  });
  const data = await response.json();
  if (!response.ok || !data.ok) throw new Error(data.error || 'Calculation failed');
  return data.result;
}

// Converts the visual expression to a safe, backend-evaluated sequence.
// Arithmetic precedence and parentheses are handled by the shunting-yard parser below.
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
      if (!Number.isFinite(n)) throw new Error('Invalid number');
      tokens.push({type:'number', value:n}); i=j; continue;
    }
    if (/[a-z]/i.test(s[i])) {
      let j=i; while(j<s.length && /[a-z]/i.test(s[j])) j++;
      const word=s.slice(i,j).toLowerCase();
      if (['pi','e'].includes(word)) tokens.push({type:'constant', value:word});
      else throw new Error('Use a function button for scientific functions');
      i=j; continue;
    }
    if ('+-*/^()%'.includes(s[i])) { tokens.push({type:'operator', value:s[i]}); i++; continue; }
    throw new Error('Unsupported character');
  }
  return tokens;
}
function toRpn(tokens) {
  const out=[], stack=[];
  const prec={'u-':4,'^':3,'*':2,'/':2,'%':2,'+':1,'-':1};
  let prev='start';
  for (const t of tokens) {
    if (t.type==='number' || t.type==='constant') { out.push(t); prev='value'; }
    else if (t.value==='(') { stack.push(t); prev='left'; }
    else if (t.value===')') {
      while(stack.length && stack[stack.length-1].value!=='(') out.push(stack.pop());
      if(!stack.length) throw new Error('Mismatched parentheses');
      stack.pop(); prev='value';
    } else {
      let op=t.value;
      if(op==='-' && (prev==='start'||prev==='operator'||prev==='left')) op='u-';
      while(stack.length && stack[stack.length-1].value!=='(' &&
        ((op!=='^' && prec[stack[stack.length-1].value]>=prec[op]) ||
         (op==='^' && prec[stack[stack.length-1].value]>prec[op]))) out.push(stack.pop());
      stack.push({type:'operator',value:op}); prev='operator';
    }
  }
  while(stack.length) {
    const t=stack.pop(); if(t.value==='(') throw new Error('Mismatched parentheses'); out.push(t);
  }
  return out;
}
async function evaluateExpression(input) {
  const rpn=toRpn(tokenize(input)), stack=[];
  for(const t of rpn) {
    if(t.type==='number') stack.push(t.value);
    else if(t.type==='constant') stack.push(await api(t.value,0));
    else {
      const op=t.value;
      if(op==='u-') { if(!stack.length) throw new Error('Invalid expression'); stack.push(-stack.pop()); continue; }
      if(stack.length<2) throw new Error('Invalid expression');
      const b=stack.pop(), a=stack.pop();
      const map={'+':'add','-':'subtract','*':'multiply','/':'divide','^':'power','%':'mod'};
      stack.push(await api(map[op],a,b));
    }
  }
  if(stack.length!==1) throw new Error('Invalid expression');
  return stack[0];
}
async function calculateUnary(op) {
  if (!expression) throw new Error('Enter a number first');
  const value = await evaluateExpression(expression);
  const result = await api(op, value);
  const old = expression;
  const formatted = format(result);
  expression = formatted; // Keep the numeric result so further operations can be chained.
  resultEl.textContent = formatted;
  statusEl.textContent = 'C BACKEND · SUCCESS';
  history.push({expression:old + ' → ' + op, result:formatted});
  renderHistory(); render();
}
function format(n) {
  if (!Number.isFinite(n)) return 'Error';
  return Number(n.toPrecision(12)).toString();
}
async function equals() {
  if (!expression) return;
  const old = expression;
  try {
    const result = await evaluateExpression(old);
    const formatted = format(result);
    resultEl.textContent = formatted;
    statusEl.textContent = 'C BACKEND · SUCCESS';
    history.push({expression:old, result:formatted});
    if(history.length>20) history.shift();
    renderHistory();
    expression = formatted;
    render();
  } catch (err) { showError(err.message); }
}
document.querySelectorAll('[data-insert]').forEach(btn => btn.addEventListener('click', () => insert(btn.dataset.insert)));
document.querySelectorAll('[data-action]').forEach(btn => btn.addEventListener('click', () => calculateUnary(btn.dataset.action).catch(e=>showError(e.message))));
document.querySelectorAll('[data-command]').forEach(btn => btn.addEventListener('click', () => {
  if(btn.dataset.command==='clear') clearAll();
  if(btn.dataset.command==='delete') deleteLast();
  if(btn.dataset.command==='equals') equals();
}));
angleToggle.addEventListener('click', () => {
  angle = angle==='DEG' ? 'RAD' : 'DEG';
  angleToggle.textContent = angle;
  document.getElementById('modeLabel').textContent = angle;
});
document.getElementById('clearHistory').addEventListener('click', () => { history=[]; renderHistory(); });
document.addEventListener('keydown', e => {
  if(/^[0-9.+\-*/^()%]$/.test(e.key)) insert(e.key);
  else if(e.key==='Enter' || e.key==='=') { e.preventDefault(); equals(); }
  else if(e.key==='Backspace') deleteLast();
  else if(e.key==='Escape') clearAll();
});
render();
