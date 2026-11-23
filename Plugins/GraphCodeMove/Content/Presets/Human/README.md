# Human pose presets

`HumanBody.gcmove` is GraphCode Move's library of human pose presets: `$Stance`, `$Back`, `$Head`, `$UpperArm`, `$Forearm`, `$ArmReach`, `$Hang`, `$Hips`, `$FootAt`, `$Thigh`, `$Shin`, `$LegReach`, `$Foot` and `$Hand`. Each poses one body part from a few values, and each value has a range.

To use it, put this one line in the header of your own `.gcmove` and call the presets in your clips. Nothing is pasted:

    presets:/GraphCodeMove/Presets/Human/HumanBody

To start, copy `Starter_Keys.gcmove` (four keys of a right hook) and `Starter.gcmove` (the hook played from those keys) into your project's Content folder. Build the keys first, then the action.

Some header lines are made per character and are not shipped: `character:` and `part:` (your MetaHuman Blueprint and its body component), `ik_rig:` (an IK Rig over your body's mesh), `controls:` (your body's `.gcrig` with each joint's limits and axes) and `physics_asset:`. The starter names this project's own; replace them with yours, and set `into:` and the starter's `source=` lines to your folder.

The presets use a MetaHuman body's bone names. Their `rig_` numbers (bone lengths, rest angles, joint stops) were measured on the MetaHuman ClothedReference body. A different body passes its own, for example `$Back(lean=20, rig_rest_tilt=-4.1)`.

`HumanFace.gcmove` is the face's presets, by region: `$Mouth`, `$Eyes`, `$Brows` and `$Cheeks`, for a part:Face file, with their controls in `HumanFace.gcrig` (the MetaHuman face's native curves). Name both in the face file's header:

    presets:/GraphCodeMove/Presets/Human/HumanFace  controls:/GraphCodeMove/Presets/Human/HumanFace

The guide is `Plugins/GraphCode/Docs/Guides/MOVE-PARAMETRIC-POSES.md`. The face: `Plugins/GraphCode/Docs/Guides/FACE-EXPRESSIONS.md`.
