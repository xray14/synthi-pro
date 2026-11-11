# SYNTHI PRO - icon generator. Draws the logo with GDI+ and writes PNG sets and a multi-size .ico into assets\icons.
#   taskbar_16/32/48/64.png   - minimal mark, readable at small sizes
#   desktop_128/256/512.png   - detailed plate with the name
#   synthipro.ico             - 16..64 minimal + 128/256 detailed (used by the installer)
# Usage: powershell -ExecutionPolicy Bypass -File tools\make_icons.ps1
$ErrorActionPreference = 'Stop'
$out = Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\icons'
New-Item -ItemType Directory -Force $out | Out-Null
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System; using System.Drawing; using System.Drawing.Drawing2D; using System.Drawing.Imaging; using System.Drawing.Text; using System.IO; using System.Collections.Generic;
public static class SpIcon {
  static GraphicsPath Round(RectangleF r, float rad) { var p = new GraphicsPath(); float d = rad * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90); p.AddArc(r.Right - d, r.Y, d, d, 270, 90); p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90); p.AddArc(r.X, r.Bottom - d, d, d, 90, 90); p.CloseFigure(); return p; }
  static Color C(int rgb) { return Color.FromArgb(255, (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255); }
  static void Face(Graphics g, RectangleF b, float[] u, int c1, int c2, bool vertical) {
    var pts = new PointF[u.Length / 2]; for (int i = 0; i < pts.Length; i++) pts[i] = new PointF(b.X + u[2 * i] * b.Width, b.Y + u[2 * i + 1] * b.Height);
    using (var br = new LinearGradientBrush(new PointF(b.X, b.Y), vertical ? new PointF(b.X, b.Bottom) : new PointF(b.Right, b.Bottom), C(c1), C(c2))) g.FillPolygon(br, pts); }
  // The mark: a bevelled triangle - brushed-steel left and bottom bars, orange right bar. Same geometry as in PluginEditor.cpp.
  public static void Mark(Graphics g, RectangleF b) {
    Face(g, b, new float[] { 0.04f,0.90f, 0.50f,0.06f, 0.50f,0.25f, 0.17f,0.82f }, 0xF4F5F6, 0x8A9096, false);   // left outer
    Face(g, b, new float[] { 0.17f,0.82f, 0.50f,0.25f, 0.50f,0.44f, 0.30f,0.74f }, 0x565C62, 0x24282C, false);   // left inner
    Face(g, b, new float[] { 0.50f,0.06f, 0.96f,0.90f, 0.83f,0.82f, 0.50f,0.25f }, 0xFFB03A, 0xE86A00, true);    // right outer
    Face(g, b, new float[] { 0.50f,0.25f, 0.83f,0.82f, 0.70f,0.74f, 0.50f,0.44f }, 0xB85400, 0x6A2E00, true);    // right inner
    Face(g, b, new float[] { 0.04f,0.90f, 0.17f,0.82f, 0.83f,0.82f, 0.96f,0.90f }, 0xD4D7DA, 0x7C8288, true);    // bottom outer
    Face(g, b, new float[] { 0.17f,0.82f, 0.30f,0.74f, 0.70f,0.74f, 0.83f,0.82f }, 0x6A7076, 0x3A3F44, true);    // bottom inner
  }
  static void Screw(Graphics g, float x, float y, float r) {
    using (var br = new LinearGradientBrush(new PointF(x - r, y - r), new PointF(x + r, y + r), C(0x6A7076), C(0x1C1F22))) g.FillEllipse(br, x - r, y - r, 2 * r, 2 * r);
    using (var p = new Pen(C(0x0C0D0E), r * 0.28f)) { g.DrawLine(p, x - r * 0.55f, y, x + r * 0.55f, y); g.DrawLine(p, x, y - r * 0.55f, x, y + r * 0.55f); } }
  public static Bitmap Render(int size, bool detailed) {
    int ss = size >= 256 ? 2 : 4, S = size * ss;
    var big = new Bitmap(S, S, PixelFormat.Format32bppArgb);
    using (var g = Graphics.FromImage(big)) {
      g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit; g.Clear(Color.Transparent);
      float m = S * 0.035f; var plate = new RectangleF(m, m, S - 2 * m, S - 2 * m); float rad = S * 0.19f;
      using (var path = Round(plate, rad)) {
        using (var br = new LinearGradientBrush(new PointF(0, 0), new PointF(0, S), C(0x34383D), C(0x121416))) g.FillPath(br, path);
        using (var br = new LinearGradientBrush(new PointF(0, 0), new PointF(S, S), C(0xB9BEC3), C(0xF08A1E)))
        using (var pen = new Pen(br, S * (detailed ? 0.014f : 0.022f))) g.DrawPath(pen, path);
      }
      if (!detailed) { float k = S * 0.70f; Mark(g, new RectangleF((S - k) / 2, (S - k) / 2 - S * 0.015f, k, k)); }
      else {
        float k = S * 0.60f; Mark(g, new RectangleF((S - k) / 2, S * 0.075f, k, k));
        float sr = S * 0.022f, so = S * 0.115f; Screw(g, so, so, sr); Screw(g, S - so, so, sr); Screw(g, so, S - so, sr); Screw(g, S - so, S - so, sr);
        using (var f = new Font("Segoe UI", S * 0.098f, FontStyle.Bold, GraphicsUnit.Pixel)) {
          var fmt = StringFormat.GenericTypographic; float w1 = g.MeasureString("SYNTHI ", f, 9999, fmt).Width + S * 0.03f, w2 = g.MeasureString("PRO", f, 9999, fmt).Width, x = (S - w1 - w2) / 2, y = S * 0.665f;
          using (var b1 = new LinearGradientBrush(new PointF(0, y), new PointF(0, y + S * 0.12f), C(0xFFFFFF), C(0x9AA0A6))) g.DrawString("SYNTHI", f, b1, x, y, fmt);
          using (var b2 = new LinearGradientBrush(new PointF(0, y), new PointF(0, y + S * 0.12f), C(0xFFB03A), C(0xE86A00))) g.DrawString("PRO", f, b2, x + w1, y, fmt);
        }
        float wy = S * 0.845f, cx = S * 0.5f;                                   // waveform under the name
        using (var pen = new Pen(C(0x8A9096), S * 0.005f)) { g.DrawLine(pen, S * 0.26f, wy, cx - S * 0.075f, wy); g.DrawLine(pen, cx + S * 0.075f, wy, S * 0.74f, wy); }
        float[] h = { 0.012f, 0.022f, 0.034f, 0.05f, 0.034f, 0.022f, 0.012f };
        using (var pen = new Pen(C(0xF08A1E), S * 0.009f)) { pen.StartCap = LineCap.Round; pen.EndCap = LineCap.Round;
          for (int i = 0; i < h.Length; i++) { float x = cx + (i - 3) * S * 0.019f; g.DrawLine(pen, x, wy - h[i] * S * 0.5f, x, wy + h[i] * S * 0.5f); } }
      }
    }
    var bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
    using (var g = Graphics.FromImage(bmp)) { g.InterpolationMode = InterpolationMode.HighQualityBicubic; g.PixelOffsetMode = PixelOffsetMode.HighQuality; g.CompositingQuality = CompositingQuality.HighQuality; g.DrawImage(big, new Rectangle(0, 0, size, size)); }
    big.Dispose(); return bmp;
  }
  public static void WriteIco(string path, string[] pngs) {
    var data = new List<byte[]>(); foreach (var p in pngs) data.Add(File.ReadAllBytes(p));
    using (var w = new BinaryWriter(File.Create(path))) {
      w.Write((short) 0); w.Write((short) 1); w.Write((short) data.Count); int off = 6 + 16 * data.Count;
      for (int i = 0; i < data.Count; i++) { int sz; using (var im = Image.FromFile(pngs[i])) sz = im.Width;
        w.Write((byte) (sz >= 256 ? 0 : sz)); w.Write((byte) (sz >= 256 ? 0 : sz)); w.Write((byte) 0); w.Write((byte) 0); w.Write((short) 1); w.Write((short) 32); w.Write(data[i].Length); w.Write(off); off += data[i].Length; }
      foreach (var d in data) w.Write(d);
    }
  }
}
"@
$files = @()
foreach ($s in 16, 32, 48, 64) { $f = Join-Path $out "taskbar_$s.png"; $b = [SpIcon]::Render($s, $false); $b.Save($f, [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose(); $files += $f }
foreach ($s in 128, 256, 512) { $f = Join-Path $out "desktop_$s.png"; $b = [SpIcon]::Render($s, $true); $b.Save($f, [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose(); if ($s -le 256) { $files += $f } }
[SpIcon]::WriteIco((Join-Path $out 'synthipro.ico'), $files)
# contact sheet for a quick visual check
$sheet = New-Object System.Drawing.Bitmap 1000, 540
$g = [System.Drawing.Graphics]::FromImage($sheet); $g.Clear([System.Drawing.Color]::FromArgb(255, 40, 44, 48)); $x = 14
foreach ($n in 'desktop_512', 'desktop_256', 'desktop_128', 'taskbar_64', 'taskbar_48', 'taskbar_32', 'taskbar_16') {
  $im = [System.Drawing.Image]::FromFile((Join-Path $out "$n.png")); $w = [Math]::Min($im.Width, 400)
  if ($n -eq 'desktop_256') { $g.DrawImage($im, 430, 14, 256, 256) } elseif ($n -eq 'desktop_512') { $g.DrawImage($im, 14, 14, 400, 400) }
  elseif ($n -eq 'desktop_128') { $g.DrawImage($im, 700, 14, 128, 128) } else { $g.DrawImage($im, 430 + $x, 300, $im.Width, $im.Height); $x += $im.Width + 20 }
  $im.Dispose() }
$g.Dispose(); $sheet.Save((Join-Path $out '_preview.png'), [System.Drawing.Imaging.ImageFormat]::Png); $sheet.Dispose()
Get-ChildItem $out | ForEach-Object { '{0,8}  {1}' -f $_.Length, $_.Name }
