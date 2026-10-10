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

highestArea = 0
largest = None
for tex in pyrenderccccc.GetTextures():
    name = pyrenderccccc.GetResourceName(tex.resourceId)
    print(f"{name} is {tex.width} x {tex.height}")
    area = tex.width * tex.height
    if area > highestArea:
        highestArea = area
        largest = tex

if largest is not None:
    name = pyrenderccccc.GetResourceName(largest.resourceId)
    print(f"\n+++ Largest texture is {name}")

    # open largest texture (by area) in texture viewer, and focus
    pyrenderccccc.ShowTextureViewer()
    pyrenderccccc.GetTextureViewer().ViewTexture(
        largest.resourceId, renderccccc.CompType.Typeless, True
    )

    # Get access to a controller to get the texture saving API access.
    # We use the blocking controller for simplicity, but a better option
    # might be to invoke onto the replay thread with
    # pyrenderccccc.Replay().AsyncInvoke()
    controller = pyrenderccccc.GetBlockingController()

    filename = pyrenderccccc.Extensions().SaveFileName(
        "Choose where to save JPG/PNG/DDS texture files", "", "*.jpg"
    )

    filename = filename.replace(".jpg", "")

    texsave = renderccccc.TextureSave()
    texsave.resourceId = largest.resourceId

    # Blend alpha to a checkerboard pattern for formats without alpha support
    texsave.alpha = renderccccc.AlphaMapping.BlendToCheckerboard

    # Most formats can only display a single image per file, so we select the
    # first mip and first slice
    texsave.mip = 0
    texsave.slice.sliceIndex = 0

    texsave.destType = renderccccc.FileType.JPG
    controller.SaveTexture(texsave, filename + ".jpg")

    # For formats with an alpha channel, preserve it
    texsave.alpha = renderccccc.AlphaMapping.Preserve

    texsave.destType = renderccccc.FileType.PNG
    controller.SaveTexture(texsave, filename + ".png")

    # DDS textures can save multiple mips and array slices, so instead
    # of the default behaviour of saving mip 0 and slice 0, we set -1
    # which saves *all* mips and slices
    texsave.mip = -1
    texsave.slice.sliceIndex = -1

    texsave.destType = renderccccc.FileType.DDS
    controller.SaveTexture(texsave, filename + ".dds")
