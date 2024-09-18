use qwt::perf_and_test_utils::{build_qwt, TimingQueries};
use qwt::utils::msb;
use qwt::utils::text_remap;
use qwt::{AccessUnsigned, HQWT256Pfs, HQWT512Pfs, HQWT256, HQWT512, HWT, WT};
use qwt::{QWT256Pfs, QWT512Pfs, QWT256, QWT512};

use clap::Parser;

const N_RUNS: usize = 10;

#[derive(Parser, Debug)]
#[clap(author, version, about, long_about = None)]
struct Args {
    /// Input filename
    #[clap(short, long, value_parser)]
    input_file: String,
}

fn main() {
    let args = Args::parse();
    let input_filename = args.input_file;
    let mut text = std::fs::read(&input_filename).expect("Cannot read the input file.");

    let sigma = text_remap(&mut text);
    let n = text.len();
    println!("Text length: {:?}", n);
    println!("Alphabet size: {sigma}");

    macro_rules! test_construction {
        ($($t: ty: $id:expr), * ) => {
            $({
                let mut t = TimingQueries::new(N_RUNS, 1);
                let mut result: u8 = 0;
                for _ in 0..N_RUNS {
                    t.start();
                    let ds = build_qwt::<$t>(&text);
                    result += unsafe { ds.get_unchecked(123) };
                    t.stop()
                }

                let (t_min, t_max, t_avg) = t.get();
                println!(
                    "RESULT algo={} input={} n={} logn={:?} min_construction_time_ms={} max_construction_time_ms={} avg_construction_time_ms={} n_runs={}",
                    stringify!($id),
                    input_filename,
                    n,
                    msb(n),
                    t_min / 1000 / 1000,
                    t_max / 1000 / 1000,
                    t_avg / 1000 / 1000,
                    N_RUNS);
                println!("Result: {result}");
            })*
        };
    }

    test_construction!(
        QWT256<_>: "QWT256",
        QWT256Pfs<_>: "QWT256Pfs",
        QWT512<_>: "QWT512",
        QWT512Pfs<_>: "QWT512Pfs",
        HQWT256<_>: "HQWT256",
        HQWT256Pfs<_>: "HQWT256Pfs",
        HQWT512<_>: "HQWT512",
        HQWT512Pfs<_>: "HQWT512Pfs",
        WT<_>: "WT",
        HWT<_>: "HWT"
    );
}
