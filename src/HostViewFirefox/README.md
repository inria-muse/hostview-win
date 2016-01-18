# HostView Addon for Firefox

HostView addon for UCN Study tracks user browsing and sends data about 
open/closed tabs to the local HostView engine for tracking the user activity
and for more detailed user questionnaires.

In addition, collects data on:

- pageload performance
- video qoe (TODO)

## Building

You need to install jpm first (and node + npm before that unless you have them already).

	$ npm install -g jpm

To build the XPI, run

	$ jpm xpi

## Testing

	$ jpm run

## Signing with Mozilla AMO

	$ jpm sign --api-key <key> --api-secret <secret>

## Installation

Open/download the xpi file in Firefox.

## Bundle with HostView installer

TODO