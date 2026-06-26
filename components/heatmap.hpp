#pragma once

// Map t∈[0,1] to a cold→hot gradient (blue → cyan → green → yellow → red),
// writing the RGB components so the caller can also pick a readable foreground.
// Shared by the heatmap toggle (capturing the range) and the grid renderer
// (shading each numeric cell).
void heat_rgb(double t, int& R, int& G, int& B);
