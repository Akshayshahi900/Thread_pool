use std::sync::mpsc;
use std::thread;

fn main() {
    let (tx, rx) = mpsc::channel();
    let handle = thread::spawn(move || {
        loop {
            match rx.recv() {
              Ok(message) => println!("Recieved: {}", message),
              Err(_) =>{
                println!("Channel Closed");
                break;
              }
            }
        }
    });
    tx.send("Job 1").unwrap();
    tx.send("Job 2").unwrap();
    tx.send("Job 3").unwrap();
    
    drop(tx);
    handle.join().unwrap();
}
