# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import re


html_theme = "rocm_docs_theme"
html_theme_options = {"flavor": "rocm"}

extensions = ["rocm_docs"]
external_toc_path = "./sphinx/_toc.yml"

html_title = f"ROCm Bandwidth Test Documentation"
project = "rocm_bandwidth_test"
author = "Advanced Micro Devices, Inc."
copyright = (
    "Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved."
)
