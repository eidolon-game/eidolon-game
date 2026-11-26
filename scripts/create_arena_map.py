import unreal

package = "/Game/Eidolon/Maps/Arena"
if not unreal.EditorAssetLibrary.does_asset_exist(package):
    unreal.EditorAssetLibrary.make_directory("/Game/Eidolon/Maps")
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if not world:
        raise RuntimeError("Failed to create arena world")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 110))
    if not start or not unreal.EditorLoadingAndSavingUtils.save_map(world, package):
        raise RuntimeError("Failed to save arena map")
unreal.log("EIDOLON arena map ready")
