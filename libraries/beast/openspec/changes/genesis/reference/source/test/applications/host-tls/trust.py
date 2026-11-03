# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Create ephemeral localhost certificates and a new Chromium trust database."""

import argparse
import hashlib
import json
import sqlite3
import subprocess
from pathlib import Path


def create_certificates(directory: Path) -> dict[str, str]:
    """Create an isolated CA and a leaf valid only for the DNS name localhost."""
    directory.mkdir(parents=True, exist_ok=True)
    if any(directory.iterdir()):
        raise ValueError("certificate directory must be empty")

    def openssl(*arguments: str) -> bytes:
        return subprocess.check_output(
            ["openssl", *arguments], cwd=directory, stderr=subprocess.DEVNULL, timeout=15)

    openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "2",
            "-subj", "/CN=WebCpp ephemeral test CA",
            "-addext", "basicConstraints=critical,CA:TRUE",
            "-addext", "keyUsage=critical,keyCertSign,cRLSign",
            "-keyout", "ca-key.pem", "-out", "ca.pem")
    openssl("req", "-new", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost",
            "-keyout", "key.pem", "-out", "request.pem")
    (directory / "extensions.cnf").write_text(
        "basicConstraints=critical,CA:FALSE\n"
        "keyUsage=critical,digitalSignature,keyEncipherment\n"
        "extendedKeyUsage=serverAuth\nsubjectAltName=DNS:localhost\n", encoding="utf-8")
    openssl("x509", "-req", "-in", "request.pem", "-CA", "ca.pem", "-CAkey", "ca-key.pem",
            "-CAcreateserial", "-days", "2", "-extfile", "extensions.cnf", "-out", "cert.pem")
    (directory / "ca.der").write_bytes(openssl("x509", "-in", "ca.pem", "-outform", "DER"))
    openssl("req", "-new", "-newkey", "rsa:2048", "-nodes",
            "-subj", "/CN=WebCpp test client", "-keyout", "client-key.pem",
            "-out", "client-request.pem")
    (directory / "client-extensions.cnf").write_text(
        "basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature\n"
        "extendedKeyUsage=clientAuth\n", encoding="utf-8")
    openssl("x509", "-req", "-in", "client-request.pem", "-CA", "ca.pem",
            "-CAkey", "ca-key.pem", "-CAcreateserial", "-days", "2",
            "-extfile", "client-extensions.cnf", "-out", "client.pem")
    (directory / "client.der").write_bytes(
        openssl("x509", "-in", "client.pem", "-outform", "DER"))
    (directory / "client-key.der").write_bytes(openssl(
        "pkcs8", "-topk8", "-nocrypt", "-in", "client-key.pem", "-outform", "DER"))
    for name in ("key.pem", "ca-key.pem", "client-key.pem", "client-key.der"):
        (directory / name).chmod(0o600)
    return {name: str((directory / file).resolve()) for name, file in (
        ("ca", "ca.pem"), ("der", "ca.der"), ("cert", "cert.pem"), ("key", "key.pem"),
        ("client_der", "client.der"), ("client_key_der", "client-key.der"))}


def seed_chrome_profile(profile: Path, certificate: Path) -> None:
    """Trust a CA only in a fresh full-Chromium profile, without global changes."""
    # Chromium 155.0.8059.39: components/server_certificate_database sources.
    # Database v1 stores CertificateMetadata {trust {trust_type: TRUSTED}}.
    # Full Chrome reads profile roots; the separate headless_shell does not.
    profile.mkdir(parents=True, exist_ok=True)
    if any(profile.iterdir()):
        raise ValueError("Chrome profile must be empty and must not be running")
    default = profile / "Default"
    default.mkdir()
    der = certificate.read_bytes()
    with sqlite3.connect(default / "ServerCertificate") as database:
        database.execute(
            "CREATE TABLE meta(key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY, value LONGVARCHAR)")
        database.executemany("INSERT INTO meta VALUES(?, ?)",
                             [("version", "1"), ("last_compatible_version", "1")])
        database.execute("CREATE TABLE certificates(sha256hash_hex TEXT PRIMARY KEY, "
                         "der_cert BLOB NOT NULL, trust_settings BLOB NOT NULL)")
        database.execute("INSERT INTO certificates VALUES(?, ?, ?)",
                         (hashlib.sha256(der).hexdigest(), der, bytes.fromhex("0a020803")))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("create", "chrome"))
    parser.add_argument("directory", type=Path)
    parser.add_argument("certificate", type=Path, nargs="?")
    arguments = parser.parse_args()
    if arguments.action == "create":
        print(json.dumps(create_certificates(arguments.directory)))
    else:
        if arguments.certificate is None:
            parser.error("chrome requires the DER CA certificate")
        seed_chrome_profile(arguments.directory, arguments.certificate)


if __name__ == "__main__":
    main()
