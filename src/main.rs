use std::sync::mpsc;
use std::thread;

type Job = Box<dyn FnOnce() + Send + 'static>;

fn main() {
   let (tx , rx) = mpsc::channel::<Job>();

   let handle = thread::spawn(move || {
    loop{
      match rx.recv() {
          Ok(job)=>{
            println!("Worker got a job");
            job();
          }
          Err(_) =>{
            println!("Worker shutting down");
            break;
          }
      }
    }
   });
    tx.send(Box::new(||{
      println!("Job 1 running");
    })).unwrap();

    
    tx.send(Box::new(||{
      println!("Job 2 running");
    })).unwrap();

    drop(tx);

    handle.join().unwrap();
}
