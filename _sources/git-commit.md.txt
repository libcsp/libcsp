# Git Commit Guidelines

## Code Changes

- Ensure code follows established coding standards.
- Avoid mixing multiple logical changes in one commit.
- Do not introduce new trailing white spaces.

For more details, refer to the [Pro Git Commit Guidelines][1].

## Commit Message Guidelines

A well-crafted commit message is crucial for effective collaboration
and code maintenance. Follow these best practices:

- **Explain the "why":** Clearly articulate the reason behind the
  change in the body of the commit message. Understanding the
  motivation is essential for reviewers and future contributors.

- **Write before summarizing:** Begin by writing the full commit
  message, explaining the details in the body, and then summarize it
  in the subject line. This ensures that both the summary and the body
  are informative.

- **Clear and concise subject line:** Use a clear and concise subject
  line that summarizes the essence of the change. It should be
  informative enough for someone to understand the purpose without
  delving into the body.

  It's not a strict rule but nice to have some "area tag" before the
  summary.  Area tags are, for example, "drivers", "bindings",
  "freertos", "zephyr", ".github", or "doc". It is an indication of
  where the commit touches.

- **Use imperative verbs:** Start the subject line with an imperative
  verb, emphasizing what the commit does rather than what it did. For
  example, use "Fix" instead of "Fixed" or "Fixes."

If you're unsure about crafting commit messages, consider reviewing
the commit history of the project. Pay attention to well-structured
messages that effectively communicate the purpose and scope of
changes. This can serve as a valuable reference and help you
understand the conventions followed by the project.

### Commit Message Examples

These are examples taken from our own commit hisotry.

```text
    drivers: usart_linux: Fail early and release unused resources

    This commit introduces early failing to csp_usart_open() in
    case neither rx_callback nor return_fd are provided.
    We also move the allocation of `ctx` into the if block ensuring
    that a rx_callback is provided. Thus, we do not allocate and
    free memory needlessly.

    After usage of the pthread attributes, we destroy them to further
    reduce memory leaks. In case of an error, at the moment we only
    log it and do not abort, as it would not influence functionality.
    If return_fd is not provided (l. 205), we do not need to explicitly
    close the descriptor as rx_callback is provided and using the fd
    in that case.

    Signed-off-by: pr0me <g33sus@gmail.com>
    Reviewed-by: Yasushi SHOJI <yashi@spacecubics.com>
```

```text
    cmake: Fix python binding option, change to py3

    The previous option name for the python bindings,
    'enable-python3-bindings' is not a valid varaible name in CMAKE.
    Also only Python3 is supported forwards so the CMake package Python3
    is now used.

    Due to the 'WITH_SOABI' option being specified, the MODULE option
    was also explicitly specified as 'WITH_SOABI' can only be used with
    'MODULE'.
```

```text
    nomtu: Remove the usage of the interface MTU field

    After leading to much confusion over the years, the mtu field has now
    been removed. This is made possible with the recent change to the
    packet structure having a compile time defined size. This means we dont
    need a runtime configuration field to check against, we can simply check
    against sizeof(packet->data) now.

    All RX functions need to check for overflow of the packet data field.
    All TX functions that is askd to transmit a csp packet larger than their
    underlaying layer cannot handle, should drop the packet.

    In-the field MTU analysis is done by sending larger and larger ping
    packets over the network to determine what the end-to-end MTU is.

    Interfaces that support "infinite mtu" are:
        CAN CFP 2.0 (begin and end flag)
        KISS (begin and end flag)
        I2C (begin and end flag)

    Interfaces that are limited:
        CAN CFP 1.0 (uses 256 frame remain field, so 2^255 * 8 = 2048 B)
        ZMQ (supports 2^29 bytes at least)
        UDP (limited to 2^16)
```

## Guidelines Commit Practices

### Merge Commits

While merge commits can be useful in some workflows, we generally
discourage their use in this project to maintain a clean and linear
commit history. Instead, please rebase your topic branch on top of the
`develop` branch.

**Example:**
```sh
git fetch origin
git rebase develop topic
```

### Micro-Commits

Micro-commits, or very small commits that fix bugs or issues within
the same pull request, can clutter the commit history and make it
difficult to follow the logical progression of changes. To maintain
clarity, please squash micro-commits into the appropriate commits by
amending the original commit that introduced the issue, rather than
creating a new commit.

**Example:**
Before merging your branch, squash micro-commits:
```sh
git rebase -i HEAD~N  # Where N is the number of commits to review
```

In the interactive rebase interface, mark micro-commits with `fixup`
or `squash` to combine them with preceding commits.

[1]: https://git-scm.com/book/en/v2/Distributed-Git-Contributing-to-a-Project#_commit_guidelines
