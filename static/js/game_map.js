let map_tiles;
let xmin, xmax, ymin, ymax;
let map;

function gameLoadMap(map_local) {
  map = map_local;
  const road_xmin = map['roads'].map(r=>'x1' in r ? Math.min(r['x0'], r['x1']) : r['x0']);
  const road_xmax = map['roads'].map(r=>'x1' in r ? Math.max(r['x0'], r['x1']) : r['x0']);
  const road_ymin = map['roads'].map(r=>'y1' in r ? Math.min(r['y0'], r['y1']) : r['y0']);
  const road_ymax = map['roads'].map(r=>'y1' in r ? Math.max(r['y0'], r['y1']) : r['y0']);
  const building_xmin = map['buildings'].map(r=>r['x']);
  const building_xmax = map['buildings'].map(r=>r['x']+r['w']);
  const building_ymin = map['buildings'].map(r=>r['y']);
  const building_ymax = map['buildings'].map(r=>r['y']+r['h']);
  xmin = Math.min(...road_xmin, ...building_xmin);
  xmax = Math.max(...road_xmax, ...building_xmax);
  ymin = Math.min(...road_ymin, ...building_ymin);
  ymax = Math.max(...road_ymax, ...building_ymax);
  const edges = [xmin, xmax, ymin, ymax];

  const def_tile = {'road': false, 'u': false, 'r': false, 'd': false, 'l': false};
  map_tiles = Array.from({length: ymax-ymin+1}, _=> Array.from({length:xmax-xmin+1}, _=>({...def_tile})));

  for(const r of map['roads']) {
    const rh = 'x1' in r;
    const ss = rh ? r['x0'] : r['y0'];
    const ee = rh ? r['x1'] : r['y1'];
    const s = Math.min(ss,ee);
    const e = Math.max(ss,ee);
    for(z=s;z<=e;++z) {
      const x = (rh ? z : r['x0']) - xmin;
      const y = (rh ? r['y0'] : z) - ymin;
      map_tiles[y][x]['road'] = true;
      if (z!=s) {
        map_tiles[y][x][rh ? 'l' : 'u'] = true;
      }
      if (z!=e) {
        map_tiles[y][x][rh ? 'r' : 'd'] = true;
      }
    }
  }
}