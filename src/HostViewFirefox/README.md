# HostView Addon for Firefox

HostView addon for UCN Study tracks user browsing behavior and
send data about open/closed tabs to the local HostView engine, or
if missing, directly to a data upload server.

## Building

$ npm install -g jpm
$ jpm xpi

## Signing with Mozilla AMO

Will be required starting from FF43.0. Create AMO account, upload the file for signing. Should pass the automatic
validation, so the process is instant. More info at:

https://wiki.mozilla.org/Addons/Extension_Signing
https://blog.mozilla.org/addons/2015/02/10/extension-signing-safer-experience/

For signing, you'll need an account at:

https://addons.mozilla.org/en-US/developers/

## Installation

Open/download the xpi file in Firefox.
