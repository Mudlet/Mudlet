# Linux packages

Builds Mudlet as a `.deb` for Ubuntu and Debian and a `.rpm` for Fedora, and serves
them as signed apt and dnf repositories from a Cloudflare R2 bucket.

## Building a package locally

Needs Docker and a checkout with its submodules. Nothing is installed on the host.

```sh
CI/linux-packages/build deb --base debian:13 -j 8
CI/linux-packages/build rpm --base fedora:44 --version 5.0.1 -o out
```

Without `--version` the package is a snapshot of the checkout, versioned so that the
next release sorts above it. Each build ends by installing the package into a clean
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
mudlet.asc                               signing key
deb/dists/<codename>/InRelease           apt index, signed
deb/pool/<codename>/*.deb
rpm/mudlet.repo                          dnf repository definition
rpm/fedora/<release>/<arch>/repodata/    dnf index, repomd.xml signed
rpm/fedora/<release>/<arch>/*.rpm        signed
```

Users install with:

```sh
curl -fsSL <base url>/install.sh | sudo sh
```

`CI/linux-packages/publish` does the signing, assembly and upload, and can be run
locally against a test key and `python3 -m http.server` to try a change.

## One-time setup

1. Create an R2 bucket and connect a custom domain to it. Use a domain in its own
   Cloudflare zone with Bot Fight Mode off, as apt and dnf cannot pass a challenge.
2. Create an R2 API token with Object Read & Write on that bucket only.
3. Create the signing key, with no passphrase so CI can use it:
   ```sh
   export GNUPGHOME="$(mktemp -d)"
   gpg --batch --passphrase '' --quick-gen-key "Mudlet package signing" rsa4096 sign never
   gpg --armor --export-secret-keys > mudlet-signing-key.asc
   ```
   Keep an offline copy: every installed system trusts this key, so replacing it
   means every user has to fetch the new one.
4. Add to the repository's Actions settings:

   | Kind     | Name                                  | Value                                   |
   |----------|---------------------------------------|-----------------------------------------|
   | secret   | `LINUX_PACKAGES_SIGNING_KEY`          | contents of `mudlet-signing-key.asc`    |
   | secret   | `LINUX_PACKAGES_R2_ACCESS_KEY_ID`     | from the R2 API token                   |
   | secret   | `LINUX_PACKAGES_R2_SECRET_ACCESS_KEY` | from the R2 API token                   |
   | variable | `R2_ACCOUNT_ID`                       | Cloudflare account ID                   |
   | variable | `LINUX_PACKAGES_R2_BUCKET`            | bucket name                             |
   | variable | `LINUX_PACKAGES_BASE_URL`             | `https://` and the custom domain        |
