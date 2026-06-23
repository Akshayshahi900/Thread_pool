use std::{
    sync::{Arc, Mutex, mpsc::{self}}, thread::{self, Thread},
};
use std::time::Duration;
//type declaration of JOB
type Job = Box<dyn FnOnce() + Send + 'static>;

// declaration of worker
struct Worker {
    id: usize,
    thread: std::thread::JoinHandle<()>,
}

impl Worker {
    fn new(id: usize, receiver: Arc<Mutex<mpsc::Receiver<Job>>>) -> Worker {
        let thread = thread::spawn(move || {
            loop {
                let message = receiver.lock().unwrap().recv();

                match message {
                    Ok(job) => {
                        println!("worker {id} got a job");
                        job();
                    }
                    Err(_) => {
                        println!("Worker {id} shutting down");
                        break;
                    }
                }
            }
        });
        return Worker { id, thread };
    }
}

struct ThreadPool {
    workers: Vec<Worker>,
    sender: mpsc::Sender<Job>,
}
impl ThreadPool {
  fn new(size:usize) -> Self {
    assert!(size >0);

    let (sender , receiver) = mpsc::channel::<Job>();

    let receiver = Arc::new(Mutex::new(receiver));

    let mut workers = Vec::with_capacity(size);

    for id in 0..size {
      workers.push(
        Worker::new(id , Arc::clone(&receiver)),
      );
    }

    ThreadPool { workers, sender }
  }
}
impl ThreadPool{
  pub fn execute<F>(&self  , f:F)
  where F: FnOnce() + Send + 'static,
  {let job = Box::new(f);

    self.sender.send(job).unwrap();

  }
}
impl Drop for ThreadPool{
  fn drop(&mut self){
    drop(self.sender);
    for worker in & mut self.workers{
      println!("Shutting down worker {}" , worker.id);
    }
  }
}

fn main() {
    let pool = ThreadPool::new(4);
    
    println!("Pool created");
    pool.execute(|| {
      println!("Job A");
    });
    pool.execute(|| {
      println!("Job B");
    });

    pool.execute(|| {
      println!("Job C");
    });
    
thread::sleep(Duration::from_secs(4));

}
