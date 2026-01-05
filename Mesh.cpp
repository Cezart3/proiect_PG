#include "Mesh.hpp"
namespace gps {

	/* Mesh Constructor */
	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures, glm::vec3 Ka, glm::vec3 Kd, glm::vec3 Ks) {

		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
        this->Ka = Ka;
        this->Kd = Kd;
        this->Ks = Ks;

		this->setupMesh();
	}

	Buffers Mesh::getBuffers() {
	    return this->buffers;
	}

	/* Mesh drawing function - also applies associated textures */
	void Mesh::Draw(gps::Shader shader)	{

		shader.useShaderProgram();

        // Send material properties
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Ka"), 1, &Ka[0]);
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Kd"), 1, &Kd[0]);
        glUniform3fv(glGetUniformLocation(shader.shaderProgram, "Ks"), 1, &Ks[0]);

        // Check if we have textures
        int hasTexture = 0;
        if (textures.size() > 0) hasTexture = 1;
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "hasTexture"), hasTexture);

		//set textures
		for (GLuint i = 0; i < textures.size(); i++) {

			glActiveTexture(GL_TEXTURE0 + i);
			glUniform1i(glGetUniformLocation(shader.shaderProgram, this->textures[i].type.c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

		glBindVertexArray(this->buffers.VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

        for(GLuint i = 0; i < this->textures.size(); i++) {

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    }

	// Initializes all the buffer objects/arrays
	void Mesh::setupMesh() {

		// Create buffers/arrays
		glGenVertexArrays(1, &this->buffers.VAO);
		glGenBuffers(1, &this->buffers.VBO);
		glGenBuffers(1, &this->buffers.EBO);

		glBindVertexArray(this->buffers.VAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->buffers.VBO);
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffers.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
}
