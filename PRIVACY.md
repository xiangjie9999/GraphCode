# GraphCode Privacy Policy

> **DRAFT for legal review, 23 November 2026.** Written from what the software actually sends.
> Items in `[brackets]` are for the owner to complete.

**Who we are:** [legal name and address of the GraphCode owner], contact admin@graphcode.dev.

## The short version

GraphCode sends us only what it needs to check a licence, offer updates and download Market
packs. It never sends us your project's content, and it never calls an AI provider. Your AI client
reads your project through GraphCode and sends what it reads to the AI provider *you* configured,
under that provider's privacy terms.

## What we receive, and why

| When | What we receive | Why |
|---|---|---|
| You request a trial on graphcode.dev/start or the signup form | The email you type, and your name, studio and message if you give them. Our host adds your country (from your IP address) and the page you came from. | To give you a trial file and reply to you. |
| First activation, or moving a licence to another computer | Your email, the activation code if you bought one, and a public key that identifies this computer. | To register the computer for your trial or licence. |
| Before a write, when no recent token covers it | Your email, licence id, this computer's public key, and a signature proving the computer holds its private key. | To issue the short-lived token that lets the write go ahead. |
| When the editor starts, then every 24 hours (you can switch this off) | A signed proof of this computer. No version, project or usage data. | To offer you updates. |
| When you search or install Market packs | Your search, and which pack you download. | To serve the catalogue and the pack. |

We do **not** receive: your assets, `.gc` documents, recipes, file or project names, screenshots,
usage statistics or crash reports. GraphCode keeps some diagnostics on your own computer (for
example merge records under `Saved/GraphCode/Shadow`); they never leave it, and you may delete them.

## Where it is processed

The licence and update services run on Supabase. The website, signup form and Market catalogue run
on Cloudflare. [Owner: confirm processors and regions, and add the payment provider when selling opens.]

## How long we keep it

[Owner: set retention, for example: licence and trial records for the life of the licence plus
[N] years; signup requests for [N] months.] Trial records are kept so that each email and computer
receives one trial.

## Your choices and rights

- You can switch off update checks in Project Settings > Plugins > GraphCode Updates.
- You can ask us for a copy of your data, to correct it, or to delete it, at admin@graphcode.dev.
  Deleting licence records ends the licence on that email.
- [Owner: add the rights statement for the jurisdictions you sell into, such as GDPR and UK GDPR.]

## Changes

We will post changes here and note them in the release notes.
