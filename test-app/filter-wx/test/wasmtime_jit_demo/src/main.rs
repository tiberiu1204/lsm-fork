use anyhow::Result;
use wasmtime::*;

fn main() -> Result<()> {
    let engine = Engine::default();
    let mut store = Store::new(&engine, ());

    let wasm = br#"
        (module
            (func $add (param i32 i32) (result i32)
                local.get 0
                local.get 1
                i32.add)
            (export "add" (func $add))
        )
    "#;

    let module = Module::new(&engine, wasm)?;
    let instance = Instance::new(&mut store, &module, &[])?;

    let add = instance.get_typed_func::<(i32, i32), i32>(&mut store, "add")?;

    let _result = add.call(&mut store, (42, 58))?;
    println!("Wasm demo finishsed");

    Ok(())
}

