# GraphCode End-User Licence Agreement

> **DRAFT for legal review, 23 November 2026.** This text describes how GraphCode actually works,
> in plain language. It has not yet been reviewed by a lawyer and is not yet in force. Items in
> `[brackets]` are for the owner to complete.

**Licensor:** [legal name of the GraphCode owner or company], [address] ("we", "us").
**Software:** GraphCode, its plugins (GraphCode Core, Play, Move, World, Shape, Rig, Groom, Garment,
Core Extension, Integrate), the bridge program, documentation, and updates we provide ("GraphCode").

By installing or using GraphCode you agree to this agreement. If you use it for an organisation,
you agree on its behalf and confirm you may do so.

## 1. What you may do

1. **Trial.** You may use every GraphCode product free for 14 days from the first activation on a
   computer. One trial is available per email address and one per computer.
2. **Free products.** You may use GraphCode Core, Shape and Integrate free for personal use, with no
   time limit, on one computer at a time.
3. **Licence.** After you buy a licence, you may use the products it names, without a time limit,
   on one computer at a time per licence. An individual licence is for one named person. A studio
   licence is for one named user per seat within your organisation.
4. **Sharing the download.** You may copy and share the unmodified download (for example with a
   teammate). Each person who uses it needs their own trial, free use or licence.
5. **Your work is yours.** Everything you create with GraphCode belongs to you: assets, `.gc`
   documents, recipe files (`.gcshape`, `.gcmove`, `.gcrig`, `.gcgroom`, `.gcgarment`, `.gchuman`,
   `.gcw` and others) and games. We claim no rights in it, and you may ship it in any product,
   commercial or not. Small runtime modules that GraphCode places in a packaged game may be
   distributed inside that game.
6. **Public headers.** You may use the published C++ headers and build rules to write code that
   calls GraphCode.

## 2. What you may not do

1. Remove, bypass or interfere with the licence checks, or use GraphCode beyond the trial, the free
   products or the licences you hold.
2. Decompile, disassemble or reverse-engineer GraphCode, except where the law allows it despite
   this restriction.
3. Sell, rent or sublicense GraphCode itself, or distribute modified copies.
4. Use GraphCode or its documentation to build a competing tool for authoring Unreal Engine
   content as text. (Your own content, including your `.gc` documents and recipes, stays yours under
   section 1.5.)

## 3. Licence checks and data

Writes need a short-lived token from our licence service, which identifies the licence and the
computer by its public key. Reading, rendering and checks do not. GraphCode does not send your
project content to us, and it does not call any AI provider. Your AI client sends what it reads to
the provider you choose, under that provider's terms. Details: [PRIVACY.md](PRIVACY.md).

## 4. Updates

We may offer updates through the editor. Nothing installs unless you choose to install it. A
licence includes updates for the engine version it was bought for [owner: confirm the update and
engine-upgrade policy, for example "all updates for the life of the major version"].

## 5. Market packs

Packs in the GraphCode Market are offered under the licence shown on each pack. Unless a pack says
otherwise, you may use its content in your projects and games on the same terms as section 1.5.

## 6. Third-party components and Unreal Engine

GraphCode includes third-party components under their own licences, listed in
[THIRD-PARTY-NOTICES.md](Plugins/GraphCode/THIRD-PARTY-NOTICES.md). Unreal Engine is licensed to you
by Epic Games under its own EULA. Some features rely on Epic services (for example MetaHuman), whose
terms apply to that use. GraphCode is not affiliated with or endorsed by Epic Games.

## 7. No warranty

GraphCode is provided "as is". We work hard to make it safe (dry runs, refusals, version restore,
recoverable deletion), but we do not guarantee it is free of errors or suitable for every purpose.
Keep backups and use version control. Where the law does not allow warranties to be excluded,
they are limited to the minimum it permits.

## 8. Limitation of liability

To the extent the law allows, we are not liable for indirect or consequential losses, or for lost
data, profits or work. Our total liability is limited to the amount you paid for GraphCode in the
twelve months before the claim. Nothing here limits liability that cannot be limited by law.

## 9. Ending this agreement

You may stop using GraphCode at any time. If you materially breach this agreement and do not fix it
within 30 days of notice, your licence ends. Sections 1.4, 2, 7, 8 and 10 survive. If a licence
ends, the content you made stays yours.

## 10. General

[Governing law and courts: owner to choose.] If part of this agreement cannot be enforced, the
rest still applies. This is the whole agreement about GraphCode between you and us; a written,
signed agreement with a studio overrides it where the two differ.

Contact: support@graphcode.dev
