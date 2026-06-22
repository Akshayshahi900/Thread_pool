use std::{
  sync::{mpsc , Arc , Mutex},
  thread,
};
type Job = Box<dyn FnOnce() + Send + 'static>;

struct Worker{
  id:usize,
  thread:std::thread::JoinHandle<()>,
}

impl Worker{
  fn new(
    id:usize , receiver :Arc<Mutex<mpsc::Receiver<Job>>>,
  ) ->Worker{
    let thread = thread::spawn(move ||{
      loop{
        let message = receiver.lock().unwrap().recv();

        match message{
          Ok(job) =>{
            println!("worker {id} got a job");
            job();
          }
          Err(_) =>{
            println!("Worker {id} shutting down");
            break;
          }
        }
      }
    });
    Worker { id, thread }
  }
}


struct ThreadPool{
  workers:Vec<Worker>,
  sender: mpsc::Sender<Job>,
}

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
