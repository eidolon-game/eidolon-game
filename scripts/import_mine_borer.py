"""Validate the Mine Borer source bundle and import it in the UE 5.6 Editor."""

from hashlib import sha256
from itertools import permutations, product
import json
import math
from pathlib import Path
import struct
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "ContentSource/MineBorer"
DESTINATION = "/Game/Eidolon/Creatures/MineBorer"
DATA_ASSET = "DA_MineBorerPresentation"
PARTS = {"Body": "body", "Head": "head", "RearLeft": "rear_left", "RearRight": "rear_right"}
PHASES = {"windup": "S_Borer_Windup.wav", "charge": "S_Borer_Charge.wav", "impact": "S_Borer_Impact.wav"}


def glb_document(data, path):
    if len(data) < 20 or struct.unpack_from("<4sII", data) != (b"glTF", 2, len(data)):
        raise ValueError(f"Invalid glTF binary header in {path}")
    size, kind = struct.unpack_from("<I4s", data, 12)
    if kind != b"JSON" or 20 + size > len(data):
        raise ValueError(f"Invalid glTF JSON chunk in {path}")
    return json.loads(data[20:20 + size])


def convert_bounds(bounds, axes, scale):
    low, high = bounds
    converted_low, converted_high = [], []
    for axis in axes:
        index = abs(axis) - 1
        if axis > 0:
            converted_low.append(scale * low[index])
            converted_high.append(scale * high[index])
        else:
            converted_low.append(-scale * high[index])
            converted_high.append(-scale * low[index])
    return converted_low, converted_high


def bounds_error(left, right):
    return max(abs(a - b) for side_left, side_right in zip(left, right) for a, b in zip(side_left, side_right))


def resolve_axes(expected, imported):
    """Require one shared orientation and unit conversion across all four pivots."""
    candidates = []
    for scale in (1.0, .01, 100.0):
        for permutation in permutations((1, 2, 3)):
            for signs in product((-1, 1), repeat=3):
                axes = tuple(axis * sign for axis, sign in zip(permutation, signs))
                error = max(bounds_error(expected[name], convert_bounds(imported[name], axes, scale))
                            for name in PARTS)
                if error < .35:
                    candidates.append((axes, scale))
    if len(candidates) != 1:
        raise RuntimeError(f"Mine Borer imported bounds do not identify a unique assembly orientation: {candidates}")
    return candidates[0]


def read_source(directory=SOURCE):
    manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
    hinge = manifest["rear_open_from_rest"]
    if (len(hinge["axis"]) != 3 or not math.isclose(math.hypot(*hinge["axis"]), 1, abs_tol=.00001)
            or not 0 < hinge["radians"] < math.pi):
        raise ValueError("Invalid rear plate rotation in the source bundle.")
    entries = {part["name"]: part for part in manifest["parts"]}
    if len(manifest["parts"]) != len(PARTS) or entries.keys() != PARTS.keys():
        raise ValueError("The Mine Borer bundle must contain one mesh for each of the four pivots.")
    for name, entry in entries.items():
        path = directory / f"SM_MineBorer_{name}.glb"
        contents = path.read_bytes()
        if entry["file"] != path.name or sha256(contents).hexdigest() != entry["sha256"]:
            raise ValueError(f"Asset checksum or name mismatch for {name}")
        scene = glb_document(contents, path)
        primitives = scene["meshes"][0]["primitives"]
        if len(scene["meshes"]) != 1 or any("uri" in image for image in scene["images"]):
            raise ValueError(f"Expected a single mesh with embedded images in {path}")
        axes = [scene["accessors"][prim["attributes"]["POSITION"]] for prim in primitives]
        low = [min(axis["min"][index] for axis in axes) for index in range(3)]
        high = [max(axis["max"][index] for axis in axes) for index in range(3)]
        expected = entry["local_bounds_cm"]
        if bounds_error(convert_bounds((low, high), (3, -1, 2), 100), (expected["min"], expected["max"])) > .03:
            raise ValueError(f"Local glTF bounds do not match the assembly for {name}")
        normal_mapped = [prim for prim in primitives if "normalTexture" in scene["materials"][prim["material"]]]
        if len(normal_mapped) != entry["textured_primitives_with_tangents"] or any(
                not {"TEXCOORD_0", "TANGENT"} <= prim["attributes"].keys() for prim in normal_mapped):
            raise ValueError(f"Missing UV or tangent data in {name}")
        triangles = sum(scene["accessors"][prim["indices"]]["count"] // 3 for prim in primitives)
        if triangles != entry["triangles"]:
            raise ValueError(f"Triangle count mismatch for {name}")
        if len(entry["pivot_cm"]) != 3 or not all(map(math.isfinite, entry["pivot_cm"])):
            raise ValueError(f"Missing {name} pivot")
    return entries, hinge


def verify_calibration(entries):
    expected = {name: (entry["local_bounds_cm"]["min"], entry["local_bounds_cm"]["max"])
                for name, entry in entries.items()}
    for axes, scale in (((1, 2, 3), 1.0), ((3, -1, 2), 100.0), ((-2, 3, 1), .01)):
        inverse = tuple(next(i if value > 0 else -i for i, value in enumerate(axes, 1)
                             if abs(value) == axis) for axis in range(1, 4))
        observed = {name: convert_bounds(bounds, inverse, 1.0 / scale) for name, bounds in expected.items()}
        if resolve_axes(expected, observed) != (axes, scale):
            raise ValueError("The Mine Borer transform calibration failed to resolve an exact test fixture.")


def read_audio(directory):
    manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
    entries = {entry["phase"]: entry for entry in manifest["cues"]}
    if len(manifest["cues"]) != len(PHASES) or entries.keys() != PHASES.keys():
        raise ValueError("The Mine Borer needs one cue for each attack phase.")
    if (manifest["format"], manifest["channels"], manifest["sample_rate"]) != ("PCM16", 1, 48_000):
        raise ValueError("Mine Borer cues must use mono 48 kHz PCM16.")
    for phase, entry in entries.items():
        path = directory / PHASES[phase]
        if entry["file"] != path.name or sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
            raise ValueError(f"Audio checksum or name mismatch for {phase}")
        with wave.open(str(path), "rb") as take:
            expected = (1, 2, 48_000, entry["frames"], "NONE")
            if take.getparams()[:5] != expected:
                raise ValueError(f"Unexpected WAV format or length for {phase}")
            samples = tuple(sample[0] for sample in struct.iter_unpack("<h", take.readframes(take.getnframes())))
        if (len(samples) != entry["frames"] or not 48_000 * .15 <= len(samples) <= 48_000 * 1.5
                or max(abs(samples[0]), abs(samples[-1])) > 4 or not 3000 < max(map(abs, samples)) < 30000):
            raise ValueError(f"Invalid cue level, length or edge fade for {phase}")
    return entries


def import_asset(unreal, tools, library, path, folder, asset_type, digest):
    canonical = f"{folder}/{path.stem}"
    asset = library.load_asset(canonical) if library.does_asset_exist(canonical) else None
    if asset:
        if not isinstance(asset, asset_type) or library.get_metadata_tag(asset, "EidolonSourceSHA") != digest:
            raise RuntimeError(f"{canonical} exists with different provenance.")
        return asset

    task = unreal.AssetImportTask()
    for key, value in (("filename", str(path)), ("destination_path", folder),
                       ("automated", True), ("replace_existing", False), ("save", False)):
        task.set_editor_property(key, value)
    tools.import_asset_tasks([task])
    matching = [asset for asset in task.get_objects() if isinstance(asset, asset_type)]
    if len(matching) != 1:
        raise RuntimeError(f"Unreal import for {path.name} returned {len(matching)} matching assets.")
    asset = matching[0]
    current = asset.get_path_name().split(".")[0]
    if current != canonical:
        if not library.rename_asset(current, canonical):
            raise RuntimeError(f"Could not assign the canonical asset path {canonical}.")
        asset = library.load_asset(canonical)
    library.set_metadata_tag(asset, "EidolonSourceSHA", digest)
    return asset


def import_presentation(unreal):
    entries, hinge = read_source()
    audio = read_audio(SOURCE / "Audio")
    library = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    assets = {name: import_asset(unreal, tools, library, SOURCE / entry["file"], f"{DESTINATION}/{name}",
                                 unreal.StaticMesh, entry["sha256"])
              for name, entry in entries.items()}
    expected, observed = {}, {}
    for name, mesh in assets.items():
        slots = mesh.get_editor_property("static_materials")
        materials = [slot.get_editor_property("material_interface") for slot in slots]
        if not materials or any(material is None or material.get_name() == "DefaultMaterial" for material in materials):
            raise RuntimeError(f"Missing imported material bindings for {name}.")
        bounds = mesh.get_bounding_box()
        lower, upper = bounds.min, bounds.max
        observed[name] = ((lower.x, lower.y, lower.z), (upper.x, upper.y, upper.z))
        local = entries[name]["local_bounds_cm"]
        expected[name] = (local["min"], local["max"])
    axes, scale = resolve_axes(expected, observed)
    asset_path = f"{DESTINATION}/{DATA_ASSET}"
    presentation = library.load_asset(asset_path) if library.does_asset_exist(asset_path) else None
    if presentation is None:
        asset_class = unreal.load_class(None, "/Script/Eidolon.EidolonMineBorerPresentation")
        if asset_class is None:
            raise RuntimeError("Build EidolonEditor before importing the Mine Borer presentation.")
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", asset_class)
        presentation = tools.create_asset(DATA_ASSET, DESTINATION, asset_class, factory)
        if presentation is None:
            raise RuntimeError("Could not create the Mine Borer presentation asset.")
    for name, key in PARTS.items():
        presentation.set_editor_property(key, assets[name])
        presentation.set_editor_property(f"{key}_pivot", unreal.Vector(*entries[name]["pivot_cm"]))
    presentation.set_editor_property("imported_axes", unreal.IntVector(*axes))
    presentation.set_editor_property("import_scale", scale)
    presentation.set_editor_property("rear_hinge_axis", unreal.Vector(*hinge["axis"]))
    presentation.set_editor_property("rear_open_radians", hinge["radians"])
    for phase, entry in audio.items():
        sound = import_asset(unreal, tools, library, SOURCE / "Audio" / entry["file"], f"{DESTINATION}/Audio",
                             unreal.SoundWave, entry["sha256"])
        presentation.set_editor_property(f"{phase}_sound", sound)
    if not library.save_directory(DESTINATION, only_if_is_dirty=True, recursive=True):
        raise RuntimeError("Could not save the Mine Borer meshes, materials and presentation.")
    unreal.log(f"Imported Mine Borer from four calibrated GLBs and three phase cues, axes {axes}, scale {scale}.")


if __name__ == "__main__":
    if "--validate" in sys.argv[1:]:
        bundle, _ = read_source()
        verify_calibration(bundle)
        sounds = read_audio(SOURCE / "Audio")
        print(f"Validated Mine Borer: {len(bundle)} pivot meshes, {sum(x['triangles'] for x in bundle.values())} triangles, "
              f"{len(sounds)} phase cues.")
    else:
        import unreal

        import_presentation(unreal)
