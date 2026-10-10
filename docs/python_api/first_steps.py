if not pyrenderccccc.IsCaptureLoaded():
    filename = pyrenderccccc.Extensions().OpenFileName("Choose a capture", "", "*.rdc")

    pyrenderccccc.LoadCapture(filename, renderccccc.ReplayOptions(), filename, False, True)

eid = pyrenderccccc.CurEvent()

name = pyrenderccccc.GetEventBrowser().GetEventName(eid)

print(f"Currently we are at EID {eid} named: '{name}'")
if eid > 1:
    prevname = pyrenderccccc.GetEventBrowser().GetEventName(eid - 1)
    print(f"    > the previous event {eid-1} is named: '{prevname}'")

pipe = pyrenderccccc.CurPipelineState()

outputs = pipe.GetOutputTargets()

for idx, out in enumerate(outputs):
    if out.resource != renderccccc.ResourceId.Null():
        name = pyrenderccccc.GetResourceName(out.resource)
        print(f"Output {idx} is: {name}")

depth = pipe.GetDepthTarget()

name = pyrenderccccc.GetResourceName(depth.resource)
print(f"Depth is: {name}")