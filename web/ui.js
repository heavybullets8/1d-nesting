function collectInput(){
  const job=document.getElementById('job').value||'';
  const tubing=document.getElementById('tubing').value||'';
  const stock=document.getElementById('stock').value||'';
  const kerf=document.getElementById('kerf').value||'';
  const cuts=document.getElementById('cuts').value.trim();
  const lines=[job,tubing,stock,kerf];
  if(cuts) lines.push(...cuts.split(/\n+/));
  lines.push('');
  return lines.join('\n');
}

function runPlanner(){
  const input=collectInput();
  Module.setInput(input);
  Module.callMain([]);
  const html=Module.getOutput();
  document.getElementById('result').srcdoc=html;
}

document.getElementById('run').addEventListener('click',runPlanner);
