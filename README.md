Self-Healing Independent File Transfer (Shift)
==============================================

In high-end computing environments, remote file transfers of very large data
sets to and from computational resources are commonplace as users are typically
widely distributed across different organizations and must transfer in data to
be processed and transfer out results for further analysis.  Local transfers
of this same data across file systems are also frequently performed by
administrators to optimize resource utilization when new file systems come
on-line or storage becomes imbalanced between existing file systems.  In both
cases, files must traverse many components on their journey from source to
destination where there are numerous opportunities for performance optimization
as well as failure.  A number of tools exist for providing reliable and/or high
performance file transfer capabilities, but most either do not support local
transfers, require specific security models and/or transport applications, are
difficult for individual users to deploy, and/or are not fully optimized for
highest performance.

Shift is a framework for Self-Healing Independent File Transfer that provides
high performance and resilience for local and remote transfers through a variety
of techniques.  These include end-to-end integrity via cryptographic hashes,
throttling of transfers to prevent resource exhaustion, balancing transfers
across resources based on load and availability, and parallelization of
transfers across multiple source and destination hosts for increased redundancy
and performance.  In addition, Shift was specifically designed to accommodate
the diverse heterogeneous environments of a widespread user base with minimal
assumptions about operating environments.  In particular, Shift is unique in its
ability to provide advanced reliability and automatic single and multi-file
parallelization to any stock command-line transfer application while being
easily deployed by both individual users as well as entire organizations.

Shift includes the following features, among others:

    - support for local, LAN, and WAN transfers

    - drop-in replacement for both cp and scp (basic options only)

    - tracking of individual file operations with on-demand status

    - transfer stop and restart

    - email notification of completion, errors, and warnings

    - local and remote tar creation/extraction

    - rsync-like synchronization based on modification times and checksums

    - integrity verification of transfers with partial retransfer/resum to
      rectify corruption

    - detection of silent corruption between transfers of the same file

    - throttling based on local and remote resource utilization

    - automatic retrieval/release of files residing on DMF-managed file systems

    - automatic striping of files transferred to Lustre file systems

    - fully self-contained besides perl core and ssh

    - automatic detection and selection of higher performance transports and
      hash utilities when available including bbftp, mcp, msum, and rsync

    - automatic many-to-many parallelization of single and multi-file
      transfers with file system equivalence detection and rewriting

Shift is in active production at the NASA Advanced Supercomputing Facility
(https://www.nas.nasa.gov/hecc/support/kb/entry/300) and has facilitated
approximately 3.4M transfers over 11B files totaling 420 PB (as of Jan.
2024) since deployment in March 2012.

For full details of the Shift architecture, see
https://pkolano.github.io/papers/resilience12.pdf and
https://pkolano.github.io/papers/hust15.pdf.  For installation details,
see "INSTALL".  For usage details, see "doc/shiftc.1" (in man page format,
viewable with "nroff -man").

Questions, comments, fixes, and/or enhancements welcome.

--Paul Kolano <pkolano@gmail.com>

