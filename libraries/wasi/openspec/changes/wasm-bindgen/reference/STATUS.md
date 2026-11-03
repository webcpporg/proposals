# Status: draft, 2026-11-03

This draft records the owner's requested direction and the architectural recommendations from
the discussion. No implementation, backend compatibility measurement or repository rename has
been performed. All implementation tasks are unchecked. No reference source is claimed.

Resume by resolving design.md O1-O8, especially the accepted wasi redesign, the WIT/backend
profile and host-tool dependencies. The companion organization change is `wasm-tools`.
The BDK input is a new component built from the example's Rust wrapper, not the published
JavaScript-oriented bdk-wasm package. Keep wallet-specific APIs out of webcpp public namespaces.
