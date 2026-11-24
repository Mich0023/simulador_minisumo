#!/bin/bash
# Arma plataforma/index.html metiendo motor_sim.js dentro de plantilla.html
cd "$(dirname "$0")"
python3 - <<'PY'
sim = open('motor_sim.js').read().replace('if (typeof module !== "undefined") module.exports = SIM;', '')
html = open('plantilla.html').read()
assert '/*__SIM__*/' in html, "falta el marcador /*__SIM__*/ en plantilla.html"
open('../index.html', 'w').write(html.replace('/*__SIM__*/', sim))
print("index.html armado:", len(open('../index.html').read()) // 1024, "KB")
PY
