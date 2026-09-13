# Dataset source index

This file records public sources used for the DIFCVerilog experiments. It keeps
links and notes only; it does not redistribute benchmark RTL files.

| Dataset | Public source | License notes | Local processing notes |
|---|---|---|---|
| Hack@DAC 2018 | <https://github.com/HACK-EVENT/hackatdac18> | Check the repository license and included component licenses. | Download the upstream SystemVerilog source and convert it locally with `sv2v` before running DIFCVerilog. |
| Hack@DAC 2019 | <https://github.com/HACK-EVENT/hackatdac19> | Multi-license SoC; keep SiFive, Ariane, and component notices from the original repository. | Download the upstream SystemVerilog source and convert it locally with `sv2v` before running DIFCVerilog. |
| Hack@DAC 2021 | <https://github.com/HACK-EVENT/hackatdac21> | The upstream README states that the design includes Hack@DAC and OpenPiton licensing terms. Review both before redistribution. | Download the upstream SystemVerilog source and convert it locally with `sv2v` before running DIFCVerilog. |
| GHOST benchmarks | <https://github.com/HSTRG1/GHOST_benchmarks> | Use the license from the upstream repository. | Download the upstream RTL and prepare labels locally before running DIFCVerilog or comparison tools. |
| TrustHub AES Trojan benchmarks | TrustHub benchmark family; commonly referenced as AES_Txxx/AES-Txxx hardware-Trojan benchmarks. | Use the license and download terms from TrustHub or the original benchmark package. | Download the benchmark source from TrustHub and prepare labels locally. |
| TrustHub BasicRSA Trojan benchmarks | TrustHub benchmark family; commonly referenced as BasicRSA_Txxx/BasicRSA-Txxx hardware-Trojan benchmarks. | Use the license and download terms from TrustHub or the original benchmark package. | Download the benchmark source from TrustHub; convert VHDL inputs to Verilog locally if needed. |
| OpenCores DES/3DES | <https://opencores.org/projects/des> | Keep the original Rudolf Usselmann copyright and redistribution notice. | Download the upstream RTL and prepare labels locally. |
| OpenCores SHA cores | <https://opencores.org/projects/sha_core> | OpenCores project page lists the SHA cores project as LGPL. Retain upstream notices. | Download the upstream RTL and prepare labels locally. |

For reproducible experiments, record the exact upstream commit, release archive,
or download date. Do not include full third-party RTL unless redistribution has
been checked for that specific dataset.
