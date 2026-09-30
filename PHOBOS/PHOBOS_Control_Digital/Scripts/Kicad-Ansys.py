import json, sys
import pcbnew

board = pcbnew.LoadBoard(sys.argv[1])
WIN = [float(v) for v in sys.argv[2:6]] if len(sys.argv) >= 6 else None  # xmin ymin xmax ymax, mm, KiCad coords (Y down)

mm = pcbnew.ToMM

def inside(x, y):
    return WIN is None or (WIN[0] <= x <= WIN[2] and WIN[1] <= y <= WIN[3])

def pt(p):                                   # flip Y for Ansys
    return [round(mm(p.x), 4), round(-mm(p.y), 4)]

def lname(layer):
    return board.GetLayerName(layer)

# ---- Stackup (best effort; verify against Board Setup) ----
stackup = []
try:
    for it in board.GetDesignSettings().GetStackupDescriptor().GetList():
        stackup.append({"name": it.GetLayerName(), "type": it.GetTypeName(),
                        "thickness_mm": mm(it.GetThickness()),
                        "material": it.GetMaterial(),
                        "er": it.GetEpsilonR(), "tand": it.GetLossTangent()})
except Exception as e:
    print("stackup read failed:", e)

# ---- Traces, arcs, vias ----
traces, vias = [], []
for t in board.GetTracks():
    if isinstance(t, pcbnew.PCB_VIA):
        p = t.GetPosition()
        if inside(mm(p.x), mm(p.y)):
            try:
                pad = mm(t.GetWidth(pcbnew.F_Cu))   # KiCad 9+
            except TypeError:
                pad = mm(t.GetWidth())              # KiCad 8 and older
            vias.append({"net": t.GetNetname(), "pos": pt(p),
                         "pad_mm": pad, "drill_mm": mm(t.GetDrillValue())})
    elif isinstance(t, pcbnew.PCB_ARC):
        traces.append({"type": "arc", "layer": lname(t.GetLayer()),
                       "net": t.GetNetname(), "width_mm": mm(t.GetWidth()),
                       "start": pt(t.GetStart()), "mid": pt(t.GetMid()),
                       "end": pt(t.GetEnd())})
    else:
        s, e = t.GetStart(), t.GetEnd()
        if inside(mm(s.x), mm(s.y)) or inside(mm(e.x), mm(e.y)):
            traces.append({"type": "line", "layer": lname(t.GetLayer()),
                           "net": t.GetNetname(), "width_mm": mm(t.GetWidth()),
                           "start": pt(s), "end": pt(e)})

# ---- Filled zones ----
zones = []
for z in board.Zones():
    for layer in z.GetLayerSet().Seq():
        polys = z.GetFilledPolysList(layer)
        for i in range(polys.OutlineCount()):
            ol = polys.Outline(i)
            zones.append({"net": z.GetNetname(), "layer": lname(layer),
                          "outline": [pt(ol.CPoint(j)) for j in range(ol.PointCount())]})

# ---- Pads ----
pads = []
for p in board.GetPads():
    pos = p.GetPosition()
    if inside(mm(pos.x), mm(pos.y)):
        try:
            size = p.GetSize(pcbnew.F_Cu)           # KiCad 9+
        except TypeError:
            size = p.GetSize()
        pads.append({"ref": p.GetParentFootprint().GetReference(),
                     "pad": p.GetNumber(), "pos": pt(pos),
                     "size_mm": [mm(size.x), mm(size.y)],
                     "net": p.GetNetname()})

out = {"units": "mm", "stackup": stackup, "traces": traces,
       "vias": vias, "zones": zones, "pads": pads}
with open("board_geometry.json", "w") as f:
    json.dump(out, f, indent=2)
print(f"{len(traces)} traces, {len(vias)} vias, {len(zones)} zone polys, {len(pads)} pads")