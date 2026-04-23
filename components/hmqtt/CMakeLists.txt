idf_component_register(SRCS "hmqtt.cpp"
        PRIV_REQUIRES log mqtt
        EMBED_TXTFILES "../../certs/ca.crt" "../../certs/client.crt" "../../certs/client.key"
        INCLUDE_DIRS "include")
