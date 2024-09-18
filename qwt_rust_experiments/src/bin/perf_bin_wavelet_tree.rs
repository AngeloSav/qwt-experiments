use std::{fs, path::Path};

use clap::Parser;
use qwt::{perf_and_test_utils::TimingQueries, AccessUnsigned, RankUnsigned, HWT, WT};
use serde::{Deserialize, Serialize};

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    /// Input filename
    #[clap(short, long, value_parser)]
    input_file: String,
    #[clap(short, long, value_parser)]
    #[arg(default_value_t = 10000000)]
    n_queries: usize,
    #[arg(short, long)]
    test_correctness: bool,
    #[arg(short, long)]
    rank: bool,
    #[arg(short, long)]
    access: bool,
    #[arg(short, long)]
    select: bool,
    #[arg(short, long)]
    rank_prefetch: bool,
}
pub fn load_or_build_and_save_qwt<DS>(
    output_filename: &str,
    text: &[<DS as AccessUnsigned>::Item],
) -> DS
where
    DS: Serialize
        + for<'a> Deserialize<'a>
        + From<Vec<<DS as AccessUnsigned>::Item>>
        + AccessUnsigned,
    <DS as AccessUnsigned>::Item: Clone,
{
    let ds: DS;
    let path = Path::new(&output_filename);
    if path.exists() {
        println!(
            "The data structure already exists. Filename: {}. I'm going to load it ...",
            output_filename
        );
        let serialized = fs::read(path).unwrap();
        println!("Serialized size: {:?} bytes", serialized.len());
        ds = bincode::deserialize::<DS>(&serialized).unwrap();
    } else {
        let mut t = TimingQueries::new(1, 1); // measure building time
        t.start();
        ds = DS::from(text.to_owned());
        t.stop();
        let (t_min, _, _) = t.get();
        println!("Construction time {:?} millisecs", t_min / 1000000);

        let serialized = bincode::serialize(&ds).unwrap();
        println!("Serialized size: {:?} bytes", serialized.len());
        fs::write(path, serialized).unwrap();
    }

    ds
}

fn main() {
    let args = Args::parse();
    let input_filename = args.input_file;
    let text = std::fs::read(&input_filename).expect("Cannot read the input file.");

    let n = text.len();
    println!("Text length: {:?}", n);

    let output_filename = input_filename.clone() + ".wt";
    let ds1 = load_or_build_and_save_qwt::<WT<_>>(&output_filename, &text);

    let output_filename = input_filename.clone() + ".hwt";
    let ds2 = load_or_build_and_save_qwt::<HWT<_>>(&output_filename, &text);

    print!("\nTesting rank consistency... ");
    for (i, &symbol) in text.iter().enumerate() {
        let r1 = ds1.rank(symbol, i).unwrap();
        let r2 = ds2.rank(symbol, i).unwrap();
        assert_eq!(r1, r2);
    }
    println!("Everything is ok!\n");
}
