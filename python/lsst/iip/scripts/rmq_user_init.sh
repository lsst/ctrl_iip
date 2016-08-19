rabbitmqctl add_vhost /bunny

rabbitmqctl add_user F1 F1
rabbitmqctl add_user F2 F2
rabbitmqctl add_user F3 F3
rabbitmqctl add_user F4 F4
rabbitmqctl add_user F5 F5
rabbitmqctl add_user F6 F6
rabbitmqctl add_user F7 F7
rabbitmqctl add_user F8 F8
rabbitmqctl add_user F9 F9
rabbitmqctl add_user F10 F10
rabbitmqctl add_user F11 F11

rabbitmqctl set_permissions -p /bunny F1  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F2  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F3  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F4  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F5  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F6  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F7  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F8  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F9  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F10 '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny F11 '.*' '.*' '.*'


rabbitmqctl add_user D1 D1
rabbitmqctl add_user D2 D2
rabbitmqctl add_user D3 D3
rabbitmqctl add_user D4 D4
rabbitmqctl add_user D5 D5
rabbitmqctl add_user D6 D6
rabbitmqctl add_user D7 D7
rabbitmqctl add_user D8 D8
rabbitmqctl add_user D9 D9
rabbitmqctl add_user D10 D10
rabbitmqctl add_user D11 D11

rabbitmqctl set_permissions -p /bunny D1  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D2  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D3  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D4  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D5  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D6  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D7  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D8  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D9  '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D10 '.*' '.*' '.*'
rabbitmqctl set_permissions -p /bunny D11 '.*' '.*' '.*'


rabbitmqctl add_user FM FM
rabbitmqctl set_permissions -p /bunny FM '.*' '.*' '.*'
rabbitmqctl set_user_tags FM administrator

rabbitmqctl add_user DMCS DMCS
rabbitmqctl set_permissions -p /bunny DMCS '.*' '.*' '.*'
rabbitmqctl set_user_tags DMCS administrator





