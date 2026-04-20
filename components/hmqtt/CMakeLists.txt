idf_component_register(SRCS "hmqtt.cpp"
        PRIV_REQUIRES log mqtt
        EMBED_TXTFILES "../../certs/client.pem"
        INCLUDE_DIRS "include")
