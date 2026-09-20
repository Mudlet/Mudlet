# Linux packages

Builds Mudlet as a `.deb` for Ubuntu and Debian and a `.rpm` for Fedora, and serves
them as signed apt and dnf repositories from a Cloudflare R2 bucket.

## Building a package locally

Needs Docker and a checkout with its submodules. Nothing is installed on the host.

```sh
CI/linux-packages/build deb --base debian:13 -j 8
CI/linux-packages/build rpm --base fedora:44 --version 5.0.1 -o out
```

Without `--version` the package is a snapshot of the checkout, versioned to sort
before the release of the same version. Each build ends by installing the package into a clean
image of the target distribution and checking that Mudlet's libraries and Lua modules
all resolve, which `--skip-verify` turns off.

The distributions that can be built are the ones shipping the Qt Mudlet needs:
Ubuntu 26.04, Debian 13 and Fedora 43 and newer.

## Publishing

Once `LINUX_PACKAGES_BASE_URL` is set (see the setup below), each `Mudlet-X.Y.Z`
release makes `create-github-release.yml` start `build-linux-packages.yml`, which builds every distribution, attaches the packages to
the release and rebuilds the repositories from the packages of the latest three
releases. To publish by hand, run that workflow with the tag and `publish` ticked.

The bucket holds nothing that cannot be rebuilt from the releases:

```
install.sh                               adds the repository and installs Mudlet
mudlet.asc                               signing key, committed here too
deb/dists/<codename>/InRelease           apt index, signed
deb/pool/<codename>/*.deb
rpm/mudlet.repo                          dnf repository definition
rpm/fedora/<release>/<arch>/repodata/    dnf index, repomd.xml signed
rpm/fedora/<release>/<arch>/*.rpm        signed
index.html                               in every directory, to browse from <base url>/index.html
```

Users install with:

```sh
curl -fsSL <base url>/install.sh | sudo sh
```

`CI/linux-packages/publish` does the signing, assembly and upload, and can be run
locally against a test key and `python3 -m http.server` to try a change - put that
key's public half in `mudlet.asc` in your checkout first, as `publish` will not
sign or assemble with a key the checkout does not carry, and restore it with
`git checkout CI/linux-packages/mudlet.asc` afterwards so a test key cannot be
committed as the real one.

## The signing key

The public half is published in two places:

| Where                  | Address                                                                                 |
|------------------------|-----------------------------------------------------------------------------------------|
| the package repository | `<base url>/mudlet.asc`                                                                 |
| this git repository    | `https://raw.githubusercontent.com/Mudlet/Mudlet/<commit>/CI/linux-packages/mudlet.asc` |
| this git repository    | `https://raw.githubusercontent.com/Mudlet/Mudlet/<commit>/CI/linux-packages/mudlet.asc`   |

The two are the same bytes, which is what lets one be checked against the other:

```sh
curl -fsSL <base url>/mudlet.asc | cmp - CI/linux-packages/mudlet.asc
```

and this is the key they hold:

```
$ gpg --show-keys CI/linux-packages/mudlet.asc
pub   rsa4096 2026-09-17 [SC]
      86177D601D97BCA0F9DE78581F27952470038A10
uid                      Mudlet package signing
```

`install.sh` takes the key and the packages from the same host, which is the
usual trust model for a distribution repository: the host is trusted once, at
install time. What the second copy buys is that whoever serves the packages
cannot also choose the key that verifies them, so anything installing Mudlet
unattended and repeatedly - a Dockerfile, a provisioning script - can pin the
committed copy and not trust the endpoint for the key at all. That replaces
`install.sh` rather than adding to it: `install.sh` overwrites the keyring with
the endpoint's copy, and apt ignores a keyring no sources entry names.

```dockerfile
ADD --chmod=644 https://raw.githubusercontent.com/Mudlet/Mudlet/<commit>/CI/linux-packages/mudlet.asc /etc/apt/keyrings/mudlet.asc
RUN printf 'Types: deb\nURIs: <base url>/deb\nSuites: trixie\nComponents: main\nSigned-By: /etc/apt/keyrings/mudlet.asc\n' \
      > /etc/apt/sources.list.d/mudlet.sources \
 && apt-get update && apt-get install -y mudlet
```

For dnf the same idea is a `.repo` written in place of fetching `rpm/mudlet.repo`,
keeping `repo_gpgcheck=1` - the directive that checks the index signature rather
than the packages:

```ini
[mudlet]
name=Mudlet
baseurl=<base url>/rpm/fedora/$releasever/$basearch/
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-mudlet
```

Pin a commit, not a branch and not a release tag. The published key is whatever
the default branch holds when the release workflow runs, because the jobs that
sign and publish check out no particular ref; a release tag is not guaranteed to
carry this file at all.

### Rotating the key

`publish` signs and assembles only with the key whose public half is committed
here, comparing the whole export rather than a fingerprint, so a new key and its
`mudlet.asc` have to arrive in the same change. That is the point: a rotation
becomes a reviewed commit instead of a file quietly changing under a URL. It also
rules out publishing a copy that is merely close enough - one carrying a second
key that every install would then trust, or frozen before the key gained a
signing subkey, which would leave the published key unable to verify the
signatures made with it.

Committing it is not the whole job, though. `assemble` rebuilds the repositories
from the last three releases and checks every rpm against the current key, so the
releases signed by the old one abort the publish. Re-sign and re-attach those -
their `.sha256` files and `SHA256SUMS.txt` entries change with them - or publish
a single release for one cycle.

## One-time setup

1. Create an R2 bucket and connect a custom domain to it. Use a domain in its own
   Cloudflare zone with Bot Fight Mode off, as apt and dnf cannot pass a challenge.
2. Create an R2 API token with Object Read & Write on that bucket only.
3. Create the signing key, with no passphrase so CI can use it:
   ```sh
   export GNUPGHOME="$(mktemp -d)"
   gpg --batch --passphrase '' --quick-gen-key "Mudlet package signing" rsa4096 sign never
   gpg --armor --export-secret-keys > ~/mudlet-signing-key.asc
   ```
   Keep an offline copy: every installed system trusts this key, so replacing it
   means every user has to fetch the new one. Write it outside the checkout, as
   above - the public half, and only that, is what gets committed, as
   `CI/linux-packages/mudlet.asc`:
   ```sh
   gpg --armor --export "Mudlet package signing" > CI/linux-packages/mudlet.asc
   ```
4. Add to the repository's Actions settings:

   | Kind     | Name                                  | Value                                   |
   |----------|---------------------------------------|-----------------------------------------|
   | secret   | `LINUX_PACKAGES_SIGNING_KEY`          | contents of `mudlet-signing-key.asc`    |
   | secret   | `LINUX_PACKAGES_R2_ACCESS_KEY_ID`     | from the R2 API token                   |
   | secret   | `LINUX_PACKAGES_R2_SECRET_ACCESS_KEY` | from the R2 API token                   |
   | variable | `R2_ACCOUNT_ID`                       | Cloudflare account ID                   |
   | variable | `LINUX_PACKAGES_R2_BUCKET`            | bucket name                             |
   | variable | `LINUX_PACKAGES_BASE_URL`             | `https://` and the custom domain        |
