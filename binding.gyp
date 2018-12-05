{
  "targets": [
    {
      "target_name": "socket",
      "sources": [
        "src/socket.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
      ]
    }
  ]
}

