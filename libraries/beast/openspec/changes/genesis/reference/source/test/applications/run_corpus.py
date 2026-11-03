# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Build and drive every pinned application, retaining each stage independently."""

import argparse
import concurrent.futures
import json
import os
import platform
import shlex
import subprocess
import sys
from pathlib import Path

import inventory

ROOT = inventory.LIBRARY.parents[1]
APPLICATIONS = inventory.LIBRARY / "example/applications"


def clean_environment():
    environment = os.environ.copy()
    for name in ("CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH"):
        environment.pop(name, None)
    return environment


def invoke(command, log, timeout=180):
    with log.open("w") as output:
        output.write(shlex.join(str(word) for word in command) + "\n")
        output.flush()
        result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT,
                                env=clean_environment(), timeout=timeout, cwd=ROOT)
    return result.returncode


def context_sources(boost_source):
    machine = platform.machine()
    architecture = "arm64_aapcs" if machine in ("aarch64", "arm64") else "x86_64_sysv"
    system = "macho" if platform.system() == "Darwin" else "elf"
    directory = boost_source / "libs/context/src"
    return [directory / "posix/stack_traits.cpp", directory / "fcontext.cpp"] + [
        directory / f"asm/{operation}_{architecture}_{system}_gas.S"
        for operation in ("make", "jump", "ontop")
    ]


def compile_application(row, variant, args, dependency_objects):
    directory = args.output / variant
    directory.mkdir(parents=True, exist_ok=True)
    executable = directory / row["id"]
    command = [args.cxx, "-std=c++20", "-O0", "-g0", "-pthread",
               "-DBOOST_ASIO_HAS_CO_AWAIT=1", "-DBOOST_ASIO_HAS_STD_COROUTINE=1"]
    includes = [args.boost_include, APPLICATIONS, APPLICATIONS / variant,
                ROOT / "libs/config/include", ROOT / "libs/asio/include",
                inventory.LIBRARY / "include"]
    if row["tls"]:
        includes.append(args.openssl / "include")
        command.append("-DWEBCPP_ASIO_HAS_OPENSSL=1")
    command += [f"-I{path}" for path in includes]
    command += [str(APPLICATIONS / variant / source) for source in row["sources"]]
    if row["stackful"]:
        command += [str(path) for path in dependency_objects]
    if row["tls"]:
        command += [str(args.openssl / "libssl.a"), str(args.openssl / "libcrypto.a")]
    command += ["-o", str(executable)]
    log = directory / f"{row['id']}.compile.log"
    try:
        code = invoke(command, log)
    except subprocess.TimeoutExpired:
        code = -1
    return row["id"], variant, {
        "compile": "passed" if code == 0 else "failed",
        "run": "planned" if code == 0 else "blocked",
        "compile_command": shlex.join(command),
        "evidence": [str(log)],
        "executable": str(executable),
        "adaptations": ["controlled fixture host, port and CA", "source namespace migration"]
        if variant == "port" else ["controlled fixture host, port and CA"],
    }


def build_context(args):
    directory = args.output / "context"
    directory.mkdir(parents=True, exist_ok=True)
    objects = []
    for source in context_sources(args.boost_source):
        target = directory / (source.name + ".o")
        command = [args.cxx, f"-I{args.boost_include}", "-c", str(source), "-o", str(target)]
        if source.suffix == ".cpp":
            command[1:1] = ["-std=c++20", "-DBOOST_CONTEXT_SOURCE"]
        if invoke(command, target.with_suffix(".log")):
            raise RuntimeError(f"Boost.Context source build failed: {source}")
        objects.append(target)
    return objects


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--boost-include", type=Path, required=True)
    parser.add_argument("--boost-source", type=Path, required=True)
    parser.add_argument("--openssl", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--variant", choices=("oracle", "port"), action="append")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--no-build", action="store_true")
    args = parser.parse_args()
    for name in ("boost_include", "boost_source", "openssl", "output"):
        setattr(args, name, getattr(args, name).resolve())
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = inventory.load()
    inventory.validate(manifest)
    rows = [row for row in manifest["applications"] if not args.only or row["id"] in args.only]
    variants = args.variant or ("oracle", "port")
    report_path = args.output / "results.json"
    report = json.loads(report_path.read_text()) if report_path.exists() else {
        "upstream_commit": inventory.COMMIT,
        "profile": "native-threads-eh-on",
        "versions": {},
        "applications": {},
    }
    report["versions"]["compiler"] = subprocess.check_output(
        [args.cxx, "--version"], text=True).splitlines()[0]
    report["versions"]["platform"] = platform.platform()
    if not args.no_build:
        objects = build_context(args)
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            pending = [pool.submit(compile_application, row, variant, args, objects)
                       for row in rows for variant in variants]
            for future in concurrent.futures.as_completed(pending):
                name, variant, result = future.result()
                report["applications"].setdefault(name, {})[variant] = result
                report_path.write_text(json.dumps(report, indent=2) + "\n")
                print(f"{name} {variant}: compile {result['compile']}", flush=True)
    if args.run:
        for row in rows:
            for variant in variants:
                result = report["applications"].get(row["id"], {}).get(variant)
                if not result or result["compile"] != "passed":
                    continue
                log = args.output / variant / f"{row['id']}.run.log"
                command = [sys.executable, str(Path(__file__).with_name("driver.py")),
                           row["id"], result["executable"]]
                try:
                    code = invoke(command, log, timeout=45)
                except subprocess.TimeoutExpired:
                    code = -1
                result["run"] = "passed" if code == 0 else "failed"
                result["run_command"] = shlex.join(command)
                result["evidence"].append(str(log))
                report_path.write_text(json.dumps(report, indent=2) + "\n")
                print(f"{row['id']} {variant}: run {result['run']}", flush=True)
    failures = [f"{name}/{variant}" for name, variants in report["applications"].items()
                for variant, result in variants.items()
                if result["compile"] == "failed" or result["run"] == "failed"]
    if failures:
        print("Failed stages: " + ", ".join(failures))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
