// Compiles ../engine.cpp - the C++ behind the façade - and links the object
// into every binary this crate produces (its tests). CXX is honoured so the
// harness's compiler is the one used; c++ is the fallback. Rust links libc
// on its own, but not the C++ runtime the engine needs, so that is named
// per platform - the one line that says "there is C++ on the other side".
use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let engine_cpp = env::var("ENGINE_CPP").unwrap_or_else(|_| "../engine.cpp".to_string());
    let out = PathBuf::from(env::var("OUT_DIR").expect("cargo sets OUT_DIR"));
    let object = out.join("engine.o");
    let cxx = env::var("CXX").unwrap_or_else(|_| "c++".to_string());
    // CXX may be more than one word ("ccache g++"): split it the way a shell
    // would before handing it to the harness's `$CXX $FLAGS`.
    let mut words = cxx.split_whitespace();
    let program = words.next().expect("CXX is not empty");
    let status = Command::new(program)
        .args(words)
        .args(["-std=c++17", "-Wall", "-Wextra", "-O1", "-fPIC", "-c"])
        .arg(&engine_cpp)
        .arg("-o")
        .arg(&object)
        .status()
        .unwrap_or_else(|e| panic!("cannot run {cxx}: {e}"));
    assert!(status.success(), "{cxx} failed to compile {engine_cpp}");
    println!("cargo:rerun-if-changed={engine_cpp}");
    println!("cargo:rerun-if-env-changed=CXX");
    println!("cargo:rerun-if-env-changed=ENGINE_CPP");
    println!("cargo:rustc-link-arg={}", object.display());
    if cfg!(target_os = "macos") {
        println!("cargo:rustc-link-lib=c++");
    } else if cfg!(not(target_os = "windows")) {
        println!("cargo:rustc-link-lib=stdc++");
    }
}
