# ani

mcp-name: io.github.jnani1972/ani

**Fast code intelligence engine for AI coding agents.** Indexes an average repository in milliseconds, the Linux kernel (28M LOC) in 3 minutes. Answers structural queries in under 1ms.

This Python wrapper downloads the selected `ani` runtime set from [GitHub Releases](https://github.com/jnani1972/ani/releases) on first run and verifies it before publishing it in your OS cache directory. The set contains the native executable and authenticated integration asset, with the graph UI always embedded.

## Installation

```bash
pip install ani
# or
pipx install ani
```

There is one composition per platform: the graph UI ships in every build, so no variant selection is needed.

## Usage

```bash
ani install   # configure your coding agents
ani --help
```

## Supported platforms

| OS      | Architecture |
|---------|-------------|
| macOS   | arm64, amd64 |
| Linux   | arm64, amd64 |
| Windows | arm64, amd64 |

## Full documentation

See [github.com/jnani1972/ani](https://github.com/jnani1972/ani)
