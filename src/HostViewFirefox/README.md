# HostView Addon for Firefox

HostView addon for UCN Study tracks user browsing behavior and
send data about open/closed tabs to the local HostView engine, or
if missing, directly to a data upload server.

## Building

$ npm install -g jpm
$ jpm xpi

## Signing with Mozilla AMO

$ jpm sign --api-key <key> --api-secret <secret>

## Installation

Open/download the xpi file in Firefox.
