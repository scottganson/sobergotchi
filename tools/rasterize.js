// Rasterize SVG files to greyscale PNGs with headless Chromium (Playwright).
// Usage: node tools/rasterize.js jobs.json
// jobs.json: [{"svg": "path.svg", "png": "out.png", "scale": 4, "hide": "shade"}, ...]
// "hide" hides the <g id="..."> layer before rendering (used to split ink and shading).
const fs = require('fs');
const { chromium } = require('playwright');

(async () => {
  const jobs = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
  const browser = await chromium.launch();
  const page = await browser.newPage({ deviceScaleFactor: 1 });
  for (const job of jobs) {
    const svg = fs.readFileSync(job.svg, 'utf8');
    const w = Number((svg.match(/<svg[^>]*\swidth="([\d.]+)"/) || [])[1]);
    const h = Number((svg.match(/<svg[^>]*\sheight="([\d.]+)"/) || [])[1]);
    if (!w || !h) throw new Error(`${job.svg}: <svg> needs numeric width and height`);
    const s = job.scale || 4;
    await page.setViewportSize({ width: Math.ceil(w * s), height: Math.ceil(h * s) });
    const hide = job.hide ? `#${job.hide}{display:none}` : '';
    await page.setContent(`<!doctype html><html><head><style>html,body{margin:0;background:#fff}svg{display:block;width:${w * s}px;height:${h * s}px}${hide}</style></head><body>${svg}</body></html>`);
    await page.screenshot({ path: job.png, clip: { x: 0, y: 0, width: Math.ceil(w * s), height: Math.ceil(h * s) } });
  }
  await browser.close();
})().catch(e => { console.error(e); process.exit(1); });
