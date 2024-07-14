{
  "targets": [
    {
      "target_name": "getwindowsinfo",
      "sources": [ "getwindowsinfo.cpp" ],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "AdditionalOptions": ["/std:c++latest"]
            }
          }
        }],
        ["OS!='win'", {
          "cflags!": ["-fno-exceptions"],
          "cflags_cc!": ["-fno-exceptions"]
        }]
      ]
    }
  ]
}
