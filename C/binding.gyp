{
  "targets": [
    {
      "target_name": "radio",
      "type": "shared_library",
      "sources": ["src/achronite/ook_send.c","src/achronite/openThings.c","src/energenie/radio.c","src/energenie/hrfm69.c","src/energenie/spis.c","src/energenie/gpio_rpi.c","src/energenie/delay_posix.c"],
      "cflags": ["-Wall", "-fPIC" ],
      'defines': [],	
      "conditions": [
        [ 'OS=="mac"', {
            "xcode_settings": {
                'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++'],
                'OTHER_LDFLAGS': ['-stdlib=libc++'],
                'MACOSX_DEPLOYMENT_TARGET': '10.7' }
            }
        ]
      ]
    }
  ]
}
