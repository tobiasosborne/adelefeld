// Render frames of an animated SVG at given times with Playwright's Chromium (lane docs-readme, a check only).
//   node lanes/docs-readme/frames.mjs FILE.svg OUTDIR W H t1 t2 ...   (times in seconds; "static" = no animation)
import { createRequire } from 'module';
const require = createRequire('/home/tobias/.nvm/versions/node/v24.11.1/lib/node_modules/@playwright/cli/');
const { chromium } = require('playwright');
const [file, outdir, w, h, ...times] = process.argv.slice(2);
const browser = await chromium.launch({ args: ['--no-sandbox'], executablePath: process.env.HOME + '/.cache/ms-playwright/chromium_headless_shell-1248/chrome-headless-shell-linux64/chrome-headless-shell' });
for (const t of times) {
  const page = await browser.newPage({ viewport: { width: +w, height: +h },
                                       reducedMotion: t === 'static' ? 'reduce' : 'no-preference' });
  await page.goto('file://' + file);
  if (t !== 'static') {
    await page.evaluate((ms) => { for (const a of document.getAnimations()) { a.pause(); a.currentTime = ms; } },
                        parseFloat(t) * 1000);
  }
  await page.screenshot({ path: `${outdir}/frame_${t}.png` });
  await page.close();
}
await browser.close();
