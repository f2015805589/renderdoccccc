# these imports are not strictly necessary, but are convenient
import renderccccc
import qrenderccccc

# this is here to give autocomplete when editing the example
# in VS Code where it doesn't know about this global
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    pyrenderccccc = qrenderccccc.CaptureContext()

if not pyrenderccccc.IsCaptureLoaded():
    filename = pyrenderccccc.Extensions().OpenFileName("Choose a capture", "", "*.rdc")

    pyrenderccccc.LoadCapture(filename, renderccccc.ReplayOptions(), filename, False, True)

mems = {}

for t in pyrenderccccc.GetTextures():
    if t.memory == renderccccc.ResourceId():
        continue

    entry = (
        t.memoryOffset,
        t.memoryOffset + t.byteSize,
        f"[Texture] {pyrenderccccc.GetResourceName(t.resourceId)}",
    )

    if t.memory not in mems:
        mems[t.memory] = []

    mems[t.memory].append(entry)

for b in pyrenderccccc.GetBuffers():
    if b.memory == renderccccc.ResourceId():
        continue

    entry = (
        b.memoryOffset,
        b.memoryOffset + b.length,
        f"[Buffer]  {pyrenderccccc.GetResourceName(b.resourceId)}",
    )

    if b.memory not in mems:
        mems[b.memory] = []

    mems[b.memory].append(entry)

for m in mems:
    binds = mems[m]

    for idx, bind1 in enumerate(binds):
        for bind2 in binds[idx + 1 :]:
            if bind1[0] < bind2[0] and bind1[1] > bind2[0]:
                print(f"In memory {pyrenderccccc.GetResourceName(m)} overlap:")
                print(f"    {bind1[0]:08x} - {bind1[1]:08x}: {bind1[2]} ")
                print(f"    {bind2[0]:08x} - {bind2[1]:08x}: {bind2[2]}")
