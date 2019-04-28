{
  "targets": [
    {
      "target_name": "radio",
      "type": "shared_library",
      "sources": ["C/achronite/lock_radio.c","C/achronite/ook_send.c","C/achronite/openThings.c",
                  "C/energenie/radio.c","C/energenie/hrfm69.c","C/energenie/spis.c","C/energenie/gpio_rpi.c","C/energenie/delay_posix.c"],
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
